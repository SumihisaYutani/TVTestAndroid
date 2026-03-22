#include "BonDriverNetwork.h"
#include "utils/Logger.h"
#include <QThread>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <cstring>

BonDriverNetwork::BonDriverNetwork(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)), m_heartbeatTimer(new QTimer(this)), m_workerThread(new QThread(this)), m_worker(nullptr), m_currentSpace(TERRESTRIAL), m_currentChannel(0), m_signalLevel(0.0f), m_isInitialized(false), m_isTunerOpen(false), m_isTsStreamActive(false)
{
    // TCP接続シグナル接続
    connect(m_socket, &QTcpSocket::connected, this, &BonDriverNetwork::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &BonDriverNetwork::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &BonDriverNetwork::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &BonDriverNetwork::onSocketError);

    // ハートビートタイマー設定（プッシュ型データ受信監視用）
    m_heartbeatTimer->setSingleShot(false); // 繰り返し実行
    m_heartbeatTimer->setInterval(10000);   // 10秒間隔でチェック
    connect(m_heartbeatTimer, &QTimer::timeout, this, &BonDriverNetwork::onHeartbeatTimeout);

    // ワーカースレッド設定（UIスレッドと独立した継続コマンド送信）
    m_worker = new ContinuousCommandWorker(this);
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::started, m_worker, &ContinuousCommandWorker::run);

    // 【削除】TSストリーム受信タイマー - プッシュ型実装では不要
    // BonDriverProxyサーバーがSetChannel2完了後に自動的にTSデータを送信

    LOG_INFO("BonDriverNetwork初期化完了 (ハートビート: 10秒, ワーカースレッド: 100ms)");
}

BonDriverNetwork::~BonDriverNetwork()
{
    // ワーカースレッド停止
    if (m_worker) {
        m_worker->stopWorker();
    }
    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(3000); // 3秒でタイムアウト
    }
    
    disconnectFromServer();
}

bool BonDriverNetwork::connectToServer(const QString &host, int port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
    {
        qDebug() << "既に接続済み";
        return true;
    }

    LOG_INFO("=== 接続開始 ===");
    LOG_INFO(QString("接続先: %1:%2").arg(host).arg(port));
    LOG_INFO(QString("ソケット状態: %1").arg(m_socket->state()));

    qDebug() << "=== 接続開始 ===";
    qDebug() << "接続先:" << host << ":" << port;
    qDebug() << "ソケット状態:" << m_socket->state();

    // 5秒タイムアウト（実際のBonDriverProxyEx設定に合わせる）
    m_socket->connectToHost(host, port);

    qDebug() << "接続要求送信完了、応答待機中...";

    if (!m_socket->waitForConnected(5000))
    {
        LOG_CRITICAL("=== 接続失敗 ===");
        LOG_CRITICAL(QString("エラー内容: %1").arg(m_socket->errorString()));
        LOG_CRITICAL(QString("エラーコード: %1").arg(m_socket->error()));
        LOG_CRITICAL(QString("ソケット状態: %1").arg(m_socket->state()));

        qCritical() << "=== 接続失敗 ===";
        qCritical() << "エラー内容:" << m_socket->errorString();
        qCritical() << "エラーコード:" << m_socket->error();
        qCritical() << "ソケット状態:" << m_socket->state();

        emit errorOccurred(QString("接続失敗: %1 (エラーコード: %2)").arg(m_socket->errorString()).arg(m_socket->error()));
        return false;
    }

    qDebug() << "TCP接続成功";
    return true;
}

void BonDriverNetwork::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
    {
        stopReceiving();

        if (m_isTunerOpen)
        {
            sendCommand(eCloseTuner);
            m_isTunerOpen = false;
        }

        if (m_isInitialized)
        {
            sendCommand(eRelease);
            m_isInitialized = false;
        }

        m_socket->disconnectFromHost();

        if (m_socket->state() != QAbstractSocket::UnconnectedState)
        {
            m_socket->waitForDisconnected(3000);
        }
    }
}

bool BonDriverNetwork::selectBonDriver(const QString &bonDriver)
{
    LOG_INFO("=== サーバープロトコル調査開始 ===");
    LOG_INFO(QString("ターゲット: %1").arg(bonDriver));
    LOG_INFO(QString("調査前の接続状態: %1").arg(isConnected() ? "接続済み" : "未接続"));

    if (!isConnected())
    {
        LOG_CRITICAL("❌ サーバーに接続されていません");
        emit errorOccurred("サーバーに接続されていません");
        return false;
    }

    // 調査結果に基づく正しいBonDriverProxyプロトコル
    // 実験：サーバーが異なるプロトコルを使用している可能性がある

    // プロトコル実験フラグ（文字列ベースのSelectBonDriverをテスト）
    bool useRawDataExperiment = false;    // 生データ実験を無効化
    bool useExperimentalProtocol = false; // 実験プロトコルを無効化
    bool useSelectBonDriver = true;       // ✅ 正常TVTestログ完全再現のためSelectBonDriverを復活

    if (useSelectBonDriver)
    {
        // Step 1: SelectBonDriverコマンド（重要発見：文字列ベース）
        // C:\TV\BonDriver_Proxy_S.ini: BONDRIVER=PT-S
        // C:\TV\BonDriver_Proxy_T.ini: BONDRIVER=PT-T

        // ユーザーが選択したBonDriverを使用
        QString driverName = bonDriver; // パラメータで受け取ったBonDriver名を使用
        QByteArray driverData = driverName.toUtf8();
        driverData.append('\0'); // null terminatorを追加（正常ログで確認済み）

        LOG_INFO(QString("📺 チューナー種別テスト: %1").arg(driverName));
        LOG_INFO(QString("  地上波: PT-T (Terrestrial)"));
        LOG_INFO(QString("  衛星: PT-S (Satellite)"));
        LOG_INFO(QString("  現在のテスト: \"%1\"").arg(driverName));
        LOG_INFO(QString("  データサイズ: %1 bytes (正常ログ: 5 bytes)").arg(driverData.size()));

        // 正常ログとの比較用HEX表示
        QString driverHex;
        for (int i = 0; i < driverData.size(); ++i)
        {
            driverHex += QString("%1 ").arg(static_cast<uint8_t>(driverData[i]), 2, 16, QChar('0'));
        }
        LOG_INFO(QString("  送信データHEX: %1").arg(driverHex.trimmed()));
        LOG_INFO(QString("  正常ログHEX: 50 54 2d 53 00"));
        if (!sendCommand(eSelectBonDriver, driverData))
        {
            LOG_CRITICAL("❌ SelectBonDriverコマンド送信失敗");
            emit errorOccurred("SelectBonDriverコマンド送信失敗");
            return false;
        }
        LOG_INFO("✅ SelectBonDriverコマンド送信成功（数値インデックス形式）");

        // SelectBonDriver応答待機（実際にレスポンスを読み取る）
        QElapsedTimer timer;
        timer.start();
        bool selectBonDriverSuccess = false;

        while (timer.elapsed() < 2000 && isConnected())
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            QThread::msleep(10);

            // 実際にレスポンスデータを読み取る
            if (m_socket->bytesAvailable() > 0)
            {
                QByteArray response = m_socket->readAll();
                LOG_INFO(QString("SelectBonDriver応答: %1 bytes").arg(response.size()));

                // 正常ログと比較：ff 00 00 00 00 00 00 01 01 (9バイト)
                QString hexData;
                for (int i = 0; i < qMin(16, response.size()); ++i)
                {
                    hexData += QString(" %1").arg(static_cast<uint8_t>(response[i]), 2, 16, QChar('0'));
                }
                LOG_INFO(QString("SelectBonDriver応答HEX: %1").arg(hexData));
                LOG_INFO(QString("正常ログ応答HEX:  ff 00 00 00 00 00 00 01 01"));

                // 応答が9バイトで最後がステータス0x01なら成功
                if (response.size() >= 9 && static_cast<uint8_t>(response[8]) == 0x01)
                {
                    LOG_INFO("✅ SelectBonDriver成功応答を確認");
                    selectBonDriverSuccess = true;
                    break;
                }
                else
                {
                    LOG_WARNING(QString("⚠️ SelectBonDriver応答が想定と異なる: サイズ=%1, 最終バイト=%2")
                                    .arg(response.size())
                                    .arg(response.size() > 0 ? QString("0x%1").arg(static_cast<uint8_t>(response[response.size() - 1]), 2, 16, QChar('0')) : "N/A"));
                }
            }
        }

        if (!isConnected())
        {
            LOG_CRITICAL("⚠️ SelectBonDriver応答待機中に接続切断 → CreateBonDriverで続行");
        }
        else if (!selectBonDriverSuccess)
        {
            LOG_WARNING("⚠️ SelectBonDriver応答が正常でない → CreateBonDriverで続行");
        }
        else
        {
            LOG_INFO("✅ SelectBonDriver完了、CreateBonDriverに進む");
        }
    }
    else if (useExperimentalProtocol)
    {
        // 実験的プロトコル：より基本的なコマンドから開始
        LOG_INFO("実験的プロトコル: GetTunerNameで基本的な応答をテスト");

        // 実験1: GetTunerNameコマンド（最も基本的なコマンド）
        if (!sendCommand(eGetTunerName))
        {
            LOG_CRITICAL("❌ GetTunerNameコマンド送信失敗");
            // 失敗してもCreateBonDriverを試行
        }
        else
        {
            LOG_INFO("✅ GetTunerNameコマンド送信成功 - サーバー応答を確認");

            // 応答待機
            QElapsedTimer basicTimer;
            basicTimer.start();
            while (basicTimer.elapsed() < 1000 && isConnected())
            {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                QThread::msleep(10);

                if (m_socket->bytesAvailable() > 0)
                {
                    QByteArray response = m_socket->readAll();
                    LOG_INFO(QString("GetTunerName応答: %1 bytes").arg(response.size()));

                    // レスポンスをHEX表示
                    QString hexData;
                    for (int i = 0; i < qMin(32, response.size()); ++i)
                    {
                        hexData += QString(" %1").arg(static_cast<uint8_t>(response[i]), 2, 16, QChar('0'));
                    }
                    LOG_INFO(QString("GetTunerName応答HEX: %1").arg(hexData));
                    break;
                }
            }
        }
    }
    else if (useRawDataExperiment)
    {
        // 生データ実験：非BonDriverProxyプロトコルの可能性を調査
        LOG_INFO("生データ実験開始: 非BonDriverProxyプロトコルの可能性を調査");

        // 実験1: 単純なテキスト送信
        QByteArray textData = "HELLO\r\n";
        qint64 bytesWritten = m_socket->write(textData);
        if (bytesWritten > 0)
        {
            LOG_INFO(QString("テキストデータ送信: %1 bytes").arg(bytesWritten));
            m_socket->flush();

            // 短時間応答待機
            QElapsedTimer textTimer;
            textTimer.start();
            while (textTimer.elapsed() < 500 && isConnected())
            {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                QThread::msleep(10);

                if (m_socket->bytesAvailable() > 0)
                {
                    QByteArray response = m_socket->readAll();
                    LOG_INFO(QString("テキスト応答: %1 bytes").arg(response.size()));
                    LOG_INFO(QString("テキスト応答内容: %1").arg(QString::fromUtf8(response)));
                    break;
                }
            }
        }

        // 実験2: HTTP風リクエスト
        if (isConnected())
        {
            QByteArray httpData = "GET / HTTP/1.1\r\nHost: baruma.f5.si\r\n\r\n";
            bytesWritten = m_socket->write(httpData);
            if (bytesWritten > 0)
            {
                LOG_INFO(QString("HTTP風リクエスト送信: %1 bytes").arg(bytesWritten));
                m_socket->flush();

                // HTTP応答待機
                QElapsedTimer httpTimer;
                httpTimer.start();
                while (httpTimer.elapsed() < 1000 && isConnected())
                {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                    QThread::msleep(10);

                    if (m_socket->bytesAvailable() > 0)
                    {
                        QByteArray response = m_socket->readAll();
                        LOG_INFO(QString("HTTP応答: %1 bytes").arg(response.size()));
                        LOG_INFO(QString("HTTP応答内容: %1").arg(QString::fromUtf8(response)));
                        break;
                    }
                }
            }
        }

        // 実験終了後は通常のCreateBonDriverも試行（念のため）
        if (isConnected())
        {
            LOG_INFO("生データ実験完了、念のためCreateBonDriverも試行");
        }
    }
    else
    {
        // BonDriverProxyEx対応：自動選択機能のためSelectBonDriverスキップ
        LOG_INFO("🔄 SelectBonDriverスキップ方式に変更");
        LOG_INFO("  理由: PT-TもPT-Sも両方失敗するため");
        LOG_INFO("  多くのBonDriverProxyサーバーはCreateBonDriverから開始可能");
        LOG_INFO("  この方式でサーバー切断を回避");
    }

    // Step 1: CreateBonDriverコマンド（データなし）- SelectBonDriverスキップ版
    LOG_INFO("Step 1: CreateBonDriver送信（SelectBonDriverスキップ）");
    if (!sendCommand(eCreateBonDriver))
    {
        LOG_CRITICAL("❌ CreateBonDriverコマンド送信失敗");
        emit errorOccurred("CreateBonDriverコマンド送信失敗");
        return false;
    }
    LOG_INFO("✅ CreateBonDriverコマンド送信成功");

    // 応答待機（失敗時続行対応）
    QElapsedTimer timer2;
    timer2.start();
    bool createBonDriverFailed = false;

    while (timer2.elapsed() < 2000 && isConnected())
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(10);

        // サーバー応答チェック
        if (m_socket->bytesAvailable() > 0)
        {
            QByteArray response = m_socket->readAll();
            LOG_INFO(QString("CreateBonDriver応答: %1 bytes").arg(response.size()));

            // 応答をHEX表示
            QString hexData;
            for (int i = 0; i < qMin(16, response.size()); ++i)
            {
                hexData += QString(" %1").arg(static_cast<uint8_t>(response[i]), 2, 16, QChar('0'));
            }
            LOG_INFO(QString("応答HEX: %1").arg(hexData));

            // 失敗フラグチェック（最終バイトが0x00）
            if (response.size() >= 9 && static_cast<uint8_t>(response[8]) == 0x00)
            {
                LOG_INFO("⚠️ CreateBonDriver失敗を検出、OpenTunerで続行");
                createBonDriverFailed = true;
            }
            break;
        }
    }

    if (!isConnected() && !createBonDriverFailed)
    {
        LOG_CRITICAL("❌ CreateBonDriver応答待機中に接続切断");
        return false;
    }

    // CreateBonDriver失敗でも続行
    if (createBonDriverFailed)
    {
        LOG_INFO("🔄 CreateBonDriver失敗でもOpenTunerで続行します");
    }

    // Step 2: OpenTunerコマンド送信（SelectBonDriverスキップ版）
    LOG_INFO("Step 2: OpenTuner送信");
    if (!sendCommand(eOpenTuner))
    {
        LOG_CRITICAL("❌ OpenTunerコマンド送信失敗");
        emit errorOccurred("OpenTunerコマンド送信失敗");
        return false;
    }
    LOG_INFO("✅ OpenTunerコマンド送信成功");

    // OpenTuner応答待機
    timer2.restart();
    while (timer2.elapsed() < 2000 && isConnected())
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(10);

        // OpenTuner応答チェック
        if (m_socket->bytesAvailable() > 0)
        {
            QByteArray response = m_socket->readAll();
            LOG_INFO(QString("OpenTuner応答: %1 bytes").arg(response.size()));

            // 応答をHEX表示
            QString hexData;
            for (int i = 0; i < qMin(16, response.size()); ++i)
            {
                hexData += QString(" %1").arg(static_cast<uint8_t>(response[i]), 2, 16, QChar('0'));
            }
            LOG_INFO(QString("OpenTuner応答HEX: %1").arg(hexData));
            break;
        }
    }

    if (!isConnected())
    {
        LOG_CRITICAL("❌ OpenTuner応答待機中に接続切断");
        return false;
    }

    LOG_INFO("✅ BonDriverProxyEx初期化完了");
    m_currentBonDriver = bonDriver;
    m_isInitialized = true;
    m_isTunerOpen = true;

    return true;
}

bool BonDriverNetwork::setChannel(TuningSpace space, uint32_t channel)
{
    if (!m_isInitialized)
    {
        emit errorOccurred("BonDriverが初期化されていません");
        return false;
    }

    LOG_INFO(QString("チャンネル設定: Space=%1, Channel=%2").arg(space).arg(channel));

    // チューナーが開いていない場合は開く
    if (!m_isTunerOpen)
    {
        if (!sendCommand(eOpenTuner))
        {
            emit errorOccurred("OpenTunerコマンド送信失敗");
            return false;
        }
        m_isTunerOpen = true;
    }

    // SetChannel2コマンド（推奨版）でチャンネル設定
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Wireshark解析に基づく正確な形式（合計9バイト）
    // 注意：Wiresharkでは 00 00 00 00 00 00 00 0e 00 なので、Space=0, Channel=14の順序
    data.append(4, 0x00);                    // Space: 4バイト全て0 (00 00 00 00)
    data.append(3, 0x00);                    // Channel上位3バイト (00 00 00)
    data.append(static_cast<char>(channel)); // Channel下位1バイト (0e)
    data.append(static_cast<char>(0));       // bRelative = FALSE (00)

    LOG_INFO(QString("SetChannel2データ生成: Space=%1, Channel=%2, データサイズ=%3バイト")
                 .arg(space)
                 .arg(channel)
                 .arg(data.size()));
    LOG_INFO(QString("送信データHEX: %1").arg(QString(data.toHex(' '))));
    LOG_INFO(QString("期待データHEX: 00 00 00 00 00 00 00 %1 00 (Space=0, Channel=%2)").arg(channel, 2, 16, QChar('0')).arg(channel));

    if (!sendCommand(eSetChannel2, data))
    {
        emit errorOccurred("SetChannel2コマンド送信失敗");
        return false;
    }

    m_currentSpace = space;
    m_currentChannel = channel;

    emit channelChanged(space, channel);
    qDebug() << "チャンネル設定完了:" << "Space=" << space << "Channel=" << channel;

    return true;
}

void BonDriverNetwork::startReceiving()
{
    if (!m_isTunerOpen)
    {
        emit errorOccurred("チューナーが開かれていません");
        return;
    }

    LOG_INFO("=== プッシュ型TSストリーム受信開始 ===");
    LOG_INFO("SetChannel2完了後、サーバーが自動的にTSデータを送信します");

    m_isTsStreamActive = true;

    // 🔄 ハートビートタイマー開始（10秒後にタイムアウトチェック）
    m_heartbeatTimer->start();
    LOG_INFO("ハートビートタイマー開始: 10秒間隔でサーバー応答を監視");

    // 📡 ワーカースレッド開始（UIスレッド独立の継続コマンド送信）
    if (!m_workerThread->isRunning()) {
        m_worker->startWorker();  // ← 追加：スレッド開始前にWorkerをアクティブ化
        m_workerThread->start();
        LOG_INFO("ワーカースレッド開始: UIフリーズ影響なし継続コマンド送信");
    } else {
        m_worker->startWorker();
        LOG_INFO("ワーカー再開: 継続コマンド送信再開");
    }

    // ハイブリッド型：SetChannel2後、スレッド独立の定期コマンド送信でデータ要求
    LOG_INFO("ハイブリッド型受信モード: 独立スレッド継続コマンド + onReadyRead()処理");
    
    LOG_INFO(QString("Worker状態確認: worker=%1, tsActive=%2, connected=%3")
             .arg(m_worker ? "存在" : "なし")
             .arg(m_isTsStreamActive ? "true" : "false") 
             .arg(isConnected() ? "true" : "false"));
}

void BonDriverNetwork::stopReceiving()
{
    LOG_INFO("=== TSストリーム受信停止 ===");
    m_isTsStreamActive = false;
    
    // 🔄 ハートビートタイマー停止
    m_heartbeatTimer->stop();
    LOG_INFO("ハートビートタイマー停止");
    
    // 📡 ワーカースレッド停止
    if (m_worker) {
        m_worker->stopWorker();
        LOG_INFO("ワーカースレッド停止: 継続コマンド送信停止");
    }
}

bool BonDriverNetwork::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

float BonDriverNetwork::getSignalLevel() const
{
    return m_signalLevel;
}


void BonDriverNetwork::onConnected()
{
    qDebug() << "サーバー接続完了";
    m_receiveBuffer.clear();
    emit connected();
}

void BonDriverNetwork::onDisconnected()
{
    LOG_CRITICAL("=== サーバー切断 ===");
    LOG_CRITICAL(QString("切断理由: %1").arg(m_socket->errorString()));
    LOG_CRITICAL(QString("ソケット状態: %1").arg(m_socket->state()));

    LOG_INFO("=== サーバー切断 ===");
    // 【削除】m_tsReceiveTimer->stop(); - プッシュ型実装では不要
    m_isInitialized = false;
    m_isTunerOpen = false;
    emit disconnected();
}

void BonDriverNetwork::onReadyRead()
{
    static int readyReadCount = 0;
    static QTime lastDataTime = QTime::currentTime();
    
    readyReadCount++;
    QTime currentTime = QTime::currentTime();
    int msSinceLastData = lastDataTime.msecsTo(currentTime);
    
    QByteArray data = m_socket->readAll();
    qDebug() << "<<< データ受信:" << data.size() << "bytes";
    
    // 🔍 詳細なソケット監視ログ
    LOG_INFO(QString("📡 onReadyRead #%1 - 受信: %2 bytes, 前回からの間隔: %3 ms")
             .arg(readyReadCount).arg(data.size()).arg(msSinceLastData));
    
    if (data.isEmpty()) {
        LOG_WARNING("⚠️ onReadyReadが呼ばれましたが、データが空です");
        LOG_INFO(QString("📊 ソケット状態: %1, bytesAvailable: %2")
                 .arg(m_socket->state()).arg(m_socket->bytesAvailable()));
        return;
    }
    
    // ソケット接続状況の定期チェック
    if (readyReadCount % 1000 == 0) {
        LOG_INFO(QString("🔍 ソケット定期チェック #%1: 状態=%2, エラー=%3")
                 .arg(readyReadCount / 1000)
                 .arg(m_socket->state())
                 .arg(m_socket->errorString()));
    }

    m_receiveBuffer.append(data);
    
    lastDataTime = currentTime;
    
    // 🔄 ハートビートタイマーリセット（データ受信があったため）
    if (m_isTsStreamActive) {
        m_heartbeatTimer->start(); // タイマーを再開（10秒後にタイムアウト）
    }

    // レスポンス処理
    processResponse();
}

void BonDriverNetwork::onSocketError(QAbstractSocket::SocketError error)
{
    LOG_CRITICAL("=== ソケットエラー発生 ===");
    LOG_CRITICAL(QString("エラーコード: %1").arg(error));
    LOG_CRITICAL(QString("エラー内容: %1").arg(m_socket->errorString()));
    LOG_CRITICAL(QString("ソケット状態: %1").arg(m_socket->state()));

    qWarning() << "ソケットエラー:" << m_socket->errorString();
    emit errorOccurred(QString("ネットワークエラー: %1").arg(m_socket->errorString()));
}

// 【削除】onTsReceiveTimer() - プッシュ型実装では不要
// BonDriverProxyサーバーはSetChannel2完了後に自動的にTSデータを送信
// onReadyRead()シグナルで受信処理

// 【削除】continuousReceive() - プッシュ型実装では完全に不要
// BonDriverProxyサーバーがSetChannel2完了後に自動的にTSデータを送信
// onReadyRead()シグナルで受信処理

bool BonDriverNetwork::sendCommand(BonDriverCommand command, const QByteArray &data)
{
    if (!isConnected())
    {
        qCritical() << "コマンド送信失敗: 接続されていません";
        return false;
    }

    QString cmdName = getCommandName(command);

    LOG_INFO(QString(">>> 公式プロトコル送信: %1 (ID:%2)").arg(cmdName).arg(command));
    LOG_INFO(QString("    データサイズ: %1").arg(data.size()));

    // 正常動作するTVTestのWiresharkログ解析結果に基づく正確なプロトコル
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    // TVTest互換プロトコル構造: [ff][cmd][00 00 00][00 00][size][data] = 8バイトヘッダー
    stream << static_cast<quint8>(0xff);        // プロトコルマーカー
    stream << static_cast<quint8>(command);     // コマンド番号
    stream << static_cast<quint8>(0x00);        // パディング1
    stream << static_cast<quint8>(0x00);        // パディング2
    stream << static_cast<quint8>(0x00);        // パディング3
    stream << static_cast<quint8>(0x00);        // パディング4
    stream << static_cast<quint8>(0x00);        // パディング5
    stream << static_cast<quint8>(data.size()); // データサイズ(8バイト目)

    // データペイロード
    if (!data.isEmpty())
    {
        stream.writeRawData(data.data(), data.size());
    }

    LOG_INFO(QString("    パケット構造: ヘッダー8bytes + データ%1bytes = 合計%2bytes").arg(data.size()).arg(packet.size()));

    // HEX表示（正常TVTestログとの比較用）
    QString hexData;
    for (int i = 0; i < packet.size(); ++i)
    {
        hexData += QString("%1 ").arg(static_cast<uint8_t>(packet[i]), 2, 16, QChar('0'));
    }
    LOG_INFO(QString("    送信HEX: %1").arg(hexData.trimmed()));

    // 正常TVTestログとの比較メモ
    if (command == eSelectBonDriver && !data.isEmpty())
    {
        LOG_INFO("    正常ログ: ff 00 00 00 00 00 00 05 50 54 2d 53 00");
    }
    else if (command == eCreateBonDriver)
    {
        LOG_INFO("    正常ログ: ff 01 00 00 00 00 00 00");
    }
    else if (command == eOpenTuner)
    {
        LOG_INFO("    正常ログ: ff 02 00 00 00 00 00 00");
    }

    qint64 bytesWritten = m_socket->write(packet);
    bool success = (bytesWritten == packet.size());

    if (success)
    {
        LOG_INFO(QString("✅ コマンド送信成功: %1 (%2 bytes)").arg(cmdName).arg(bytesWritten));
        m_socket->flush(); // 強制送信
    }
    else
    {
        LOG_CRITICAL(QString("❌ コマンド送信失敗: %1").arg(cmdName));
        LOG_CRITICAL(QString("    期待サイズ: %1, 実際の送信: %2").arg(packet.size()).arg(bytesWritten));
        LOG_CRITICAL(QString("    ソケット状態: %1").arg(m_socket->state()));
        LOG_CRITICAL(QString("    ソケットエラー: %1").arg(m_socket->errorString()));
    }

    return success;
}

void BonDriverNetwork::processResponse()
{
    // バッファサイズ制限（10MBを超えたら強制クリア）
    const int maxBufferSize = 10 * 1024 * 1024; // 10MB
    if (m_receiveBuffer.size() > maxBufferSize) {
        LOG_WARNING(QString("⚠️ 受信バッファ制限超過 (%1 MB) - バッファクリア").arg(m_receiveBuffer.size() / 1024 / 1024));
        m_receiveBuffer.clear();
        return;
    }
    
    // バースト分のTSパケットを1つにまとめてから一括送信
    QByteArray tsBatch;

    while (m_receiveBuffer.size() > 0)
    {
        // コマンドレスポンス(0xff)検出 → 蓄積済みTSを先に送信してからコマンド処理
        if (m_receiveBuffer.size() >= 8 && static_cast<uint8_t>(m_receiveBuffer[0]) == 0xff)
        {
            if (!tsBatch.isEmpty()) {
                emit tsDataReceived(tsBatch);
                tsBatch.clear();
            }
            if (!processCommandResponse()) {
                m_receiveBuffer.remove(0, 1);
            }
            continue;
        }

        // TSパケット処理
        if (m_receiveBuffer.size() >= 188) {
            // TS同期バイト(0x47)チェック
            if (static_cast<uint8_t>(m_receiveBuffer[0]) != 0x47) {
                int syncPos = -1;
                for (int s = 1; s < m_receiveBuffer.size() - 188; s++) {
                    if (static_cast<uint8_t>(m_receiveBuffer[s]) == 0x47 &&
                        static_cast<uint8_t>(m_receiveBuffer[s + 188]) == 0x47) {
                        syncPos = s;
                        break;
                    }
                }
                if (syncPos > 0) {
                    m_receiveBuffer.remove(0, syncPos);
                } else {
                    m_receiveBuffer.remove(0, 1);
                }
                continue;
            }

            tsBatch.append(m_receiveBuffer.left(188));
            m_receiveBuffer.remove(0, 188);
            continue;
        }

        // 188バイト未満は次回まで待機
        break;
    }

    // バースト末尾の残りTSを送信
    if (!tsBatch.isEmpty()) {
        emit tsDataReceived(tsBatch);
    }
}

bool BonDriverNetwork::processCommandResponse()
{
    if (m_receiveBuffer.size() < 8)
    {
        return false; // ヘッダーが不完全
    }

    uint8_t responseCmd = static_cast<uint8_t>(m_receiveBuffer[1]);

    // 【重要修正】GetTsStreamレスポンス: 8バイトヘッダーだけ削除してTSデータをバッファに残す
    // bytes[2..5]のサイズフィールドはLE/BEの違いにより誤読されるため使用しない
    // 後続のTSデータはprocessResponse()のTS同期処理(0x47検出)で正しく処理される
    if (responseCmd == eGetTsStream)
    {
        m_isTsStreamActive = true;
        m_receiveBuffer.remove(0, 8); // ヘッダー8バイトのみ削除

        static int tsHeaderCount = 0;
        if (++tsHeaderCount <= 10 || tsHeaderCount % 100 == 0) {
            LOG_INFO(QString("📡 GetTsStreamヘッダー #%1 処理完了 - 残バッファ: %2 bytes")
                    .arg(tsHeaderCount).arg(m_receiveBuffer.size()));
        }
        return true;
    }

    // その他コマンドレスポンス: bytes[2..5] LEからサイズ取得
    uint32_t responseSize;
    memcpy(&responseSize, &m_receiveBuffer.data()[2], sizeof(uint32_t));

    if (responseSize > 10 * 1024 * 1024) { // 10MB制限
        LOG_WARNING(QString("⚠️ 異常なレスポンスサイズ: %1 bytes - 1バイト破棄").arg(responseSize));
        m_receiveBuffer.remove(0, 1);
        return false;
    }

    int totalPacketSize = 8 + responseSize; // ヘッダー8バイト + データ

    if (m_receiveBuffer.size() < totalPacketSize)
    {
        return false; // まだ完全なレスポンスが到着していない
    }

    QByteArray responseData = m_receiveBuffer.mid(8, responseSize);
    m_receiveBuffer.remove(0, totalPacketSize);

    // コマンドレスポンス処理（ログ削減）
    static int cmdResponseCount = 0;
    cmdResponseCount++;
    if (cmdResponseCount <= 5 || cmdResponseCount % 100 == 0)
    {
        LOG_INFO(QString("<<< コマンドレスポンス #%1 cmd=%2: %3 bytes")
                .arg(cmdResponseCount).arg(responseCmd).arg(responseData.size()));
    }

    // GetSignalLevel: floatを取り出して保存
    if (responseCmd == eGetSignalLevel && responseData.size() >= 4)
    {
        float signalLevel;
        memcpy(&signalLevel, responseData.data(), sizeof(float));
        m_signalLevel = signalLevel;
    }

    return true;
}

QString BonDriverNetwork::getCommandName(BonDriverCommand command) const
{
    switch (command)
    {
    case eSelectBonDriver:
        return "SelectBonDriver";
    case eCreateBonDriver:
        return "CreateBonDriver";
    case eOpenTuner:
        return "OpenTuner";
    case eCloseTuner:
        return "CloseTuner";
    case eSetChannel:
        return "SetChannel";
    case eGetSignalLevel:
        return "GetSignalLevel";
    case eWaitTsStream:
        return "WaitTsStream";
    case eGetReadyCount:
        return "GetReadyCount";
    case eGetTsStream:
        return "GetTsStream";
    case ePurgeTsStream:
        return "PurgeTsStream";
    case eRelease:
        return "Release";
    case eGetTunerName:
        return "GetTunerName";
    case eIsTunerOpening:
        return "IsTunerOpening";
    case eEnumTuningSpace:
        return "EnumTuningSpace";
    case eEnumChannelName:
        return "EnumChannelName";
    case eSetChannel2:
        return "SetChannel2";
    default:
        return QString("Unknown_%1").arg(command);
    }
}

void BonDriverNetwork::onHeartbeatTimeout()
{
    static int timeoutCount = 0;
    timeoutCount++;
    
    LOG_WARNING(QString("⚠️ ハートビートタイムアウト #%1 - 10秒間データ受信なし").arg(timeoutCount));
    
    // ソケット状態の詳細確認
    LOG_INFO(QString("📊 ソケット状態: %1").arg(m_socket->state()));
    LOG_INFO(QString("📊 ソケットエラー: %1").arg(m_socket->errorString()));
    LOG_INFO(QString("📊 bytesAvailable: %1").arg(m_socket->bytesAvailable()));
    LOG_INFO(QString("📊 TSストリーム状態: %1").arg(m_isTsStreamActive ? "Active" : "Inactive"));
    
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        LOG_CRITICAL("🚨 ソケット接続が切断されています");
        emit errorOccurred("サーバー接続が切断されました");
        return;
    }
    
    // 3回連続でタイムアウトした場合はサーバー側問題と判断
    if (timeoutCount >= 3) {
        LOG_CRITICAL(QString("🚨 連続タイムアウト %1回 - BonDriverProxyサーバーがデータ送信を停止した可能性").arg(timeoutCount));
        emit errorOccurred("サーバーからのデータ受信が長時間停止しています。サーバー側のタイムアウトまたは設定問題の可能性があります。");
        // タイマーは継続（復旧を待つ）
        return;
    }
    
    // タイマーを継続（次回10秒後にチェック）
    LOG_INFO("次のハートビートチェックまで10秒待機...");
}

bool BonDriverNetwork::sendCommandThreadSafe(BonDriverCommand command, const QByteArray &data)
{
    // ワーカースレッドから安全にコマンド送信
    // QTcpSocketはスレッドセーフなのでそのまま呼び出し可能
    return sendCommand(command, data);
}

// =============================================================================
// ContinuousCommandWorker Implementation
// =============================================================================

ContinuousCommandWorker::ContinuousCommandWorker(BonDriverNetwork* parent)
    : m_bonDriver(parent), m_running(false), m_stopRequested(false)
{
}

void ContinuousCommandWorker::startWorker()
{
    QMutexLocker locker(&m_mutex);
    m_running = true;
    m_stopRequested = false;
    m_condition.wakeAll();
}

void ContinuousCommandWorker::stopWorker()
{
    QMutexLocker locker(&m_mutex);
    m_stopRequested = true;
    m_running = false;
    m_condition.wakeAll();
}

void ContinuousCommandWorker::run()
{
    int commandCount = 0;
    int commandCycle = 0; // 0:eGetTsStream, 1:ePurgeTsStream, 2:eGetSignalLevel
    
    while (true) {
        QMutexLocker locker(&m_mutex);
        
        // 停止要求をチェック
        if (m_stopRequested) {
            break;
        }
        
        // アクティブ状態まで待機
        if (!m_running) {
            LOG_DEBUG("Worker待機: m_running = false");
            m_condition.wait(&m_mutex, 1000);
            continue;
        }
        if (!m_bonDriver->isTsStreamActiveForWorker()) {
            LOG_DEBUG("Worker待機: isTsStreamActive = false");
            m_condition.wait(&m_mutex, 1000);
            continue;
        }
        if (!m_bonDriver->isConnected()) {
            LOG_DEBUG("Worker待機: isConnected = false");
            m_condition.wait(&m_mutex, 1000);
            continue;
        }
        
        locker.unlock(); // ロック解除してコマンド送信
        
        commandCount++;
        
        // Worker開始のデバッグログ
        if (commandCount == 1) {
            LOG_INFO("🚀 Worker処理開始: 最初のコマンド送信");
        }
        
        // 通常ストリーミング中は GetTsStream のみ使用
        // PurgeTsStream は毎サイクル呼ぶとサーバーバッファが破棄されTSデータが欠落するため除外
        // GetSignalLevel は100回に1回（10秒間隔）
        if (commandCycle == 2) {
            m_bonDriver->sendCommandThreadSafe(BonDriverNetwork::eGetSignalLevel);
            if (commandCount % 1000 == 0) {
                LOG_INFO(QString("📶 ワーカー継続コマンド #%1: eGetSignalLevel送信").arg(commandCount));
            }
            commandCycle = 0;
        } else {
            m_bonDriver->sendCommandThreadSafe(BonDriverNetwork::eGetTsStream);
            if (commandCount <= 10 || commandCount % 100 == 0) {
                LOG_INFO(QString("📡 ワーカー継続コマンド #%1: eGetTsStream送信").arg(commandCount));
            }
            // 100回に1回だけ GetSignalLevel を挟む（10秒間隔）
            commandCycle = (commandCount % 100 == 0) ? 2 : 0;
        }
        
        // 100ms待機（TVTest本家準拠）
        QThread::msleep(100);
    }
}

