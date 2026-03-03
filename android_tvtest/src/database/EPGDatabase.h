#ifndef EPGDATABASE_H
#define EPGDATABASE_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QMutex>
#include <QtCore/QTimer>

enum class EventGenre {
    News = 0,
    Sports = 1,
    Information = 2,
    Drama = 3,
    Music = 4,
    Variety = 5,
    Movie = 6,
    Anime = 7,
    Documentary = 8,
    Theater = 9,
    Education = 10,
    Welfare = 11,
    Other = 15
};

struct EPGEvent {
    int eventId;
    int serviceId;
    int networkId;
    int transportStreamId;
    QDateTime startTime;
    QDateTime endTime;
    QString title;
    QString description;
    QString extendedDescription;
    EventGenre genre;
    bool isFreeCA;
    bool isPresent;
    bool isFollowing;
    QString originalNetworkName;
    QString serviceName;
    
    // Additional metadata
    QString director;
    QString cast;
    QString narrator;
    int audioMode; // 0: mono, 1: stereo, 2: surround
    int videoType; // 0: SD, 1: HD, 2: 4K
    QString contentRating;
    
    bool isValid() const {
        return eventId > 0 && !title.isEmpty() && startTime.isValid();
    }
    
    int durationMinutes() const {
        return startTime.secsTo(endTime) / 60;
    }
};

struct ServiceInfo {
    int serviceId;
    int networkId;
    int transportStreamId;
    QString serviceName;
    QString providerName;
    int serviceType;
    QString logoUrl;
    bool isActive;
};

class EPGDatabase : public QObject
{
    Q_OBJECT

public:
    explicit EPGDatabase(QObject *parent = nullptr);
    ~EPGDatabase();

    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_isInitialized; }

    // Service management
    bool addService(const ServiceInfo &service);
    bool updateService(const ServiceInfo &service);
    bool removeService(int serviceId);
    QList<ServiceInfo> getAllServices() const;
    ServiceInfo getService(int serviceId) const;

    // EPG event management
    bool addEvent(const EPGEvent &event);
    bool updateEvent(const EPGEvent &event);
    bool removeEvent(int eventId);
    bool removeExpiredEvents();

    // EPG queries
    QList<EPGEvent> getEvents(int serviceId, const QDateTime &startTime, const QDateTime &endTime) const;
    QList<EPGEvent> getCurrentEvents() const;
    QList<EPGEvent> getNextEvents() const;
    EPGEvent getCurrentEvent(int serviceId) const;
    EPGEvent getNextEvent(int serviceId) const;
    
    // Search functionality
    QList<EPGEvent> searchEvents(const QString &keyword) const;
    QList<EPGEvent> getEventsByGenre(EventGenre genre) const;
    QList<EPGEvent> getEventsByTimeRange(const QDateTime &start, const QDateTime &end) const;

    // Statistics
    int getTotalEventCount() const;
    int getServiceEventCount(int serviceId) const;
    QDateTime getOldestEventTime() const;
    QDateTime getNewestEventTime() const;

    // Maintenance
    void vacuum();
    void analyze();
    bool backup(const QString &backupPath) const;
    bool restore(const QString &backupPath);

signals:
    void eventAdded(const EPGEvent &event);
    void eventUpdated(const EPGEvent &event);
    void eventRemoved(int eventId);
    void serviceAdded(const ServiceInfo &service);
    void serviceUpdated(const ServiceInfo &service);
    void databaseError(const QString &error);

public slots:
    void performMaintenance();

private slots:
    void cleanupExpiredEvents();

private:
    bool createTables();
    bool upgradeDatabase(int fromVersion, int toVersion);
    void setupMaintenanceTimer();
    
    QString getGenreString(EventGenre genre) const;
    EventGenre parseGenre(int genreCode) const;
    
    bool executeQuery(QSqlQuery &query, const QString &operation) const;
    void logSqlError(const QSqlError &error, const QString &operation) const;

    QSqlDatabase m_database;
    QString m_databasePath;
    mutable QMutex m_mutex;
    bool m_isInitialized;
    
    QTimer *m_maintenanceTimer;
    
    // Database version for schema upgrades
    static const int DATABASE_VERSION = 1;
    
    // Maintenance settings
    static const int MAINTENANCE_INTERVAL = 24 * 60 * 60 * 1000; // 24 hours
    static const int MAX_EVENT_DAYS = 7; // Keep events for 7 days
};

// Utility class for EPG data formatting
class EPGFormatter
{
public:
    static QString formatDuration(int minutes);
    static QString formatDateTime(const QDateTime &dateTime);
    static QString formatGenre(EventGenre genre);
    static QString formatServiceName(const ServiceInfo &service);
    static QString formatEventSummary(const EPGEvent &event);
    static QString formatTimeRange(const QDateTime &start, const QDateTime &end);
    
private:
    EPGFormatter() = delete;
};

#endif // EPGDATABASE_H