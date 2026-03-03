#include "EPGDatabase.h"
#include "../utils/Logger.h"

#include <QtSql/QSqlDriver>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>

EPGDatabase::EPGDatabase(QObject *parent)
    : QObject(parent)
    , m_isInitialized(false)
    , m_maintenanceTimer(nullptr)
{
    Logger::info("EPGDatabase: Initializing...");
    
    // Setup database path
#ifdef Q_OS_ANDROID
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString dataDir = QCoreApplication::applicationDirPath() + "/data";
#endif
    
    QDir().mkpath(dataDir);
    m_databasePath = dataDir + "/epg.db";
    
    Logger::info("EPGDatabase: Database path: " + m_databasePath);
}

EPGDatabase::~EPGDatabase()
{
    Logger::info("EPGDatabase: Shutting down...");
    shutdown();
}

bool EPGDatabase::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_isInitialized) {
        Logger::info("EPGDatabase: Already initialized");
        return true;
    }
    
    Logger::info("EPGDatabase: Starting initialization...");
    
    // Open database
    m_database = QSqlDatabase::addDatabase("QSQLITE", "epg_connection");
    m_database.setDatabaseName(m_databasePath);
    
    if (!m_database.open()) {
        Logger::error("EPGDatabase: Failed to open database: " + m_database.lastError().text());
        return false;
    }
    
    Logger::info("EPGDatabase: Database opened successfully");
    
    // Create tables if they don't exist
    if (!createTables()) {
        Logger::error("EPGDatabase: Failed to create tables");
        m_database.close();
        return false;
    }
    
    // Setup maintenance timer
    setupMaintenanceTimer();
    
    m_isInitialized = true;
    Logger::info("EPGDatabase: Initialization complete");
    
    return true;
}

void EPGDatabase::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isInitialized) {
        return;
    }
    
    Logger::info("EPGDatabase: Shutting down...");
    
    // Stop maintenance timer
    if (m_maintenanceTimer) {
        m_maintenanceTimer->stop();
        m_maintenanceTimer->deleteLater();
        m_maintenanceTimer = nullptr;
    }
    
    // Close database
    if (m_database.isOpen()) {
        m_database.close();
    }
    
    QSqlDatabase::removeDatabase("epg_connection");
    
    m_isInitialized = false;
    Logger::info("EPGDatabase: Shutdown complete");
}

bool EPGDatabase::addService(const ServiceInfo &service)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isInitialized) {
        Logger::error("EPGDatabase: Database not initialized");
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO services 
        (service_id, network_id, transport_stream_id, service_name, provider_name, 
         service_type, logo_url, is_active)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(service.serviceId);
    query.addBindValue(service.networkId);
    query.addBindValue(service.transportStreamId);
    query.addBindValue(service.serviceName);
    query.addBindValue(service.providerName);
    query.addBindValue(service.serviceType);
    query.addBindValue(service.logoUrl);
    query.addBindValue(service.isActive);
    
    if (executeQuery(query, "add service")) {
        emit serviceAdded(service);
        Logger::info("EPGDatabase: Added service: " + service.serviceName);
        return true;
    }
    
    return false;
}

bool EPGDatabase::addEvent(const EPGEvent &event)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isInitialized) {
        Logger::error("EPGDatabase: Database not initialized");
        return false;
    }
    
    if (!event.isValid()) {
        Logger::error("EPGDatabase: Invalid event data");
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO events 
        (event_id, service_id, network_id, transport_stream_id, 
         start_time, end_time, title, description, extended_description,
         genre, is_free_ca, is_present, is_following,
         director, cast_info, narrator, audio_mode, video_type, content_rating)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(event.eventId);
    query.addBindValue(event.serviceId);
    query.addBindValue(event.networkId);
    query.addBindValue(event.transportStreamId);
    query.addBindValue(event.startTime);
    query.addBindValue(event.endTime);
    query.addBindValue(event.title);
    query.addBindValue(event.description);
    query.addBindValue(event.extendedDescription);
    query.addBindValue(static_cast<int>(event.genre));
    query.addBindValue(event.isFreeCA);
    query.addBindValue(event.isPresent);
    query.addBindValue(event.isFollowing);
    query.addBindValue(event.director);
    query.addBindValue(event.cast);
    query.addBindValue(event.narrator);
    query.addBindValue(event.audioMode);
    query.addBindValue(event.videoType);
    query.addBindValue(event.contentRating);
    
    if (executeQuery(query, "add event")) {
        emit eventAdded(event);
        return true;
    }
    
    return false;
}

QList<EPGEvent> EPGDatabase::getEvents(int serviceId, const QDateTime &startTime, const QDateTime &endTime) const
{
    QMutexLocker locker(&m_mutex);
    QList<EPGEvent> events;
    
    if (!m_isInitialized) {
        return events;
    }
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT e.*, s.service_name, s.provider_name 
        FROM events e
        LEFT JOIN services s ON e.service_id = s.service_id
        WHERE e.service_id = ? AND e.start_time >= ? AND e.start_time < ?
        ORDER BY e.start_time
    )");
    
    query.addBindValue(serviceId);
    query.addBindValue(startTime);
    query.addBindValue(endTime);
    
    if (query.exec()) {
        while (query.next()) {
            EPGEvent event;
            event.eventId = query.value("event_id").toInt();
            event.serviceId = query.value("service_id").toInt();
            event.networkId = query.value("network_id").toInt();
            event.transportStreamId = query.value("transport_stream_id").toInt();
            event.startTime = query.value("start_time").toDateTime();
            event.endTime = query.value("end_time").toDateTime();
            event.title = query.value("title").toString();
            event.description = query.value("description").toString();
            event.extendedDescription = query.value("extended_description").toString();
            event.genre = static_cast<EventGenre>(query.value("genre").toInt());
            event.isFreeCA = query.value("is_free_ca").toBool();
            event.isPresent = query.value("is_present").toBool();
            event.isFollowing = query.value("is_following").toBool();
            event.serviceName = query.value("service_name").toString();
            event.director = query.value("director").toString();
            event.cast = query.value("cast_info").toString();
            event.narrator = query.value("narrator").toString();
            event.audioMode = query.value("audio_mode").toInt();
            event.videoType = query.value("video_type").toInt();
            event.contentRating = query.value("content_rating").toString();
            
            events.append(event);
        }
    } else {
        logSqlError(query.lastError(), "get events");
    }
    
    return events;
}

EPGEvent EPGDatabase::getCurrentEvent(int serviceId) const
{
    QMutexLocker locker(&m_mutex);
    EPGEvent event;
    
    if (!m_isInitialized) {
        return event;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT e.*, s.service_name 
        FROM events e
        LEFT JOIN services s ON e.service_id = s.service_id
        WHERE e.service_id = ? AND e.start_time <= ? AND e.end_time > ?
        ORDER BY e.start_time DESC
        LIMIT 1
    )");
    
    query.addBindValue(serviceId);
    query.addBindValue(now);
    query.addBindValue(now);
    
    if (query.exec() && query.next()) {
        event.eventId = query.value("event_id").toInt();
        event.serviceId = query.value("service_id").toInt();
        event.networkId = query.value("network_id").toInt();
        event.transportStreamId = query.value("transport_stream_id").toInt();
        event.startTime = query.value("start_time").toDateTime();
        event.endTime = query.value("end_time").toDateTime();
        event.title = query.value("title").toString();
        event.description = query.value("description").toString();
        event.extendedDescription = query.value("extended_description").toString();
        event.genre = static_cast<EventGenre>(query.value("genre").toInt());
        event.serviceName = query.value("service_name").toString();
    }
    
    return event;
}

QList<EPGEvent> EPGDatabase::searchEvents(const QString &keyword) const
{
    QMutexLocker locker(&m_mutex);
    QList<EPGEvent> events;
    
    if (!m_isInitialized || keyword.isEmpty()) {
        return events;
    }
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT e.*, s.service_name 
        FROM events e
        LEFT JOIN services s ON e.service_id = s.service_id
        WHERE e.title LIKE ? OR e.description LIKE ? OR e.extended_description LIKE ?
        ORDER BY e.start_time
        LIMIT 100
    )");
    
    QString searchPattern = "%" + keyword + "%";
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);
    
    if (query.exec()) {
        while (query.next()) {
            EPGEvent event;
            // ... populate event fields (same as getEvents)
            events.append(event);
        }
    }
    
    return events;
}

bool EPGDatabase::removeExpiredEvents()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isInitialized) {
        return false;
    }
    
    QDateTime cutoffTime = QDateTime::currentDateTime().addDays(-MAX_EVENT_DAYS);
    
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM events WHERE end_time < ?");
    query.addBindValue(cutoffTime);
    
    if (executeQuery(query, "remove expired events")) {
        int removedCount = query.numRowsAffected();
        Logger::info("EPGDatabase: Removed " + QString::number(removedCount) + " expired events");
        return true;
    }
    
    return false;
}

void EPGDatabase::performMaintenance()
{
    Logger::info("EPGDatabase: Performing maintenance...");
    
    // Remove expired events
    removeExpiredEvents();
    
    // Optimize database
    vacuum();
    analyze();
    
    Logger::info("EPGDatabase: Maintenance complete");
}

void EPGDatabase::cleanupExpiredEvents()
{
    removeExpiredEvents();
}

bool EPGDatabase::createTables()
{
    QSqlQuery query(m_database);
    
    // Create services table
    QString servicesTable = R"(
        CREATE TABLE IF NOT EXISTS services (
            service_id INTEGER PRIMARY KEY,
            network_id INTEGER,
            transport_stream_id INTEGER,
            service_name TEXT NOT NULL,
            provider_name TEXT,
            service_type INTEGER,
            logo_url TEXT,
            is_active BOOLEAN DEFAULT 1,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(servicesTable)) {
        logSqlError(query.lastError(), "create services table");
        return false;
    }
    
    // Create events table
    QString eventsTable = R"(
        CREATE TABLE IF NOT EXISTS events (
            event_id INTEGER PRIMARY KEY,
            service_id INTEGER,
            network_id INTEGER,
            transport_stream_id INTEGER,
            start_time TIMESTAMP NOT NULL,
            end_time TIMESTAMP NOT NULL,
            title TEXT NOT NULL,
            description TEXT,
            extended_description TEXT,
            genre INTEGER,
            is_free_ca BOOLEAN DEFAULT 0,
            is_present BOOLEAN DEFAULT 0,
            is_following BOOLEAN DEFAULT 0,
            director TEXT,
            cast_info TEXT,
            narrator TEXT,
            audio_mode INTEGER,
            video_type INTEGER,
            content_rating TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (service_id) REFERENCES services (service_id)
        )
    )";
    
    if (!query.exec(eventsTable)) {
        logSqlError(query.lastError(), "create events table");
        return false;
    }
    
    // Create indexes
    QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_events_service_time ON events (service_id, start_time)",
        "CREATE INDEX IF NOT EXISTS idx_events_time_range ON events (start_time, end_time)",
        "CREATE INDEX IF NOT EXISTS idx_events_title ON events (title)",
        "CREATE INDEX IF NOT EXISTS idx_events_genre ON events (genre)",
        "CREATE INDEX IF NOT EXISTS idx_services_active ON services (is_active)"
    };
    
    for (const QString &indexSql : indexes) {
        if (!query.exec(indexSql)) {
            logSqlError(query.lastError(), "create index");
            return false;
        }
    }
    
    Logger::info("EPGDatabase: Tables and indexes created successfully");
    return true;
}

void EPGDatabase::setupMaintenanceTimer()
{
    m_maintenanceTimer = new QTimer(this);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &EPGDatabase::performMaintenance);
    
    // Start maintenance timer (run every 24 hours)
    m_maintenanceTimer->start(MAINTENANCE_INTERVAL);
    
    Logger::info("EPGDatabase: Maintenance timer started");
}

bool EPGDatabase::executeQuery(QSqlQuery &query, const QString &operation) const
{
    if (!query.exec()) {
        logSqlError(query.lastError(), operation);
        return false;
    }
    return true;
}

void EPGDatabase::logSqlError(const QSqlError &error, const QString &operation) const
{
    QString errorMsg = "EPGDatabase: SQL error during " + operation + ": " + error.text();
    Logger::error(errorMsg);
    emit const_cast<EPGDatabase*>(this)->databaseError(errorMsg);
}

void EPGDatabase::vacuum()
{
    QSqlQuery query(m_database);
    if (query.exec("VACUUM")) {
        Logger::info("EPGDatabase: Database vacuum completed");
    }
}

void EPGDatabase::analyze()
{
    QSqlQuery query(m_database);
    if (query.exec("ANALYZE")) {
        Logger::info("EPGDatabase: Database analyze completed");
    }
}

// EPGFormatter implementation
QString EPGFormatter::formatDuration(int minutes)
{
    int hours = minutes / 60;
    int mins = minutes % 60;
    
    if (hours > 0) {
        return QString("%1時間%2分").arg(hours).arg(mins);
    } else {
        return QString("%1分").arg(mins);
    }
}

QString EPGFormatter::formatDateTime(const QDateTime &dateTime)
{
    return dateTime.toString("MM月dd日(ddd) hh:mm");
}

QString EPGFormatter::formatGenre(EventGenre genre)
{
    switch (genre) {
    case EventGenre::News: return "ニュース";
    case EventGenre::Sports: return "スポーツ";
    case EventGenre::Information: return "情報";
    case EventGenre::Drama: return "ドラマ";
    case EventGenre::Music: return "音楽";
    case EventGenre::Variety: return "バラエティ";
    case EventGenre::Movie: return "映画";
    case EventGenre::Anime: return "アニメ";
    case EventGenre::Documentary: return "ドキュメンタリー";
    case EventGenre::Theater: return "演劇";
    case EventGenre::Education: return "教育";
    case EventGenre::Welfare: return "福祉";
    default: return "その他";
    }
}

QString EPGFormatter::formatEventSummary(const EPGEvent &event)
{
    QString summary = QString("[%1] %2")
                     .arg(formatDateTime(event.startTime))
                     .arg(event.title);
    
    if (event.durationMinutes() > 0) {
        summary += QString(" (%1)").arg(formatDuration(event.durationMinutes()));
    }
    
    return summary;
}