#include "BonDriverNetwork.h"
#include "utils/Logger.h"
#include <QThread>
#include <QElapsedTimer>
#include <QCoreApplication>

BonDriverNetwork::BonDriverNetwork(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)), m_tsReceiveTimer(new QTimer(this)), m_currentSpace(TERRESTRIAL), m_currentChannel(0), m_signalLevel(0.0f), m_isInitialized(false), m_isTunerOpen(false), m_isTsStreamActive(false)
{
    // TCP接続シグナル接続
    connect(m_socket, &QTcpSocket::connected, this, &BonDriverNetwork::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &BonDriverNetwork::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &BonDriverNetwork::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &BonDriverNetwork::onSocketError);

    // TSストリーム受信タイマー設定
    m_tsReceiveTimer->setSingleShot(false);
    m_tsReceiveTimer->setInterval(1); // 1ms間隔（極限高速受信で安定化）
    connect(m_tsReceiveTimer, &QTimer::timeout, this, &BonDriverNetwork::onTsReceiveTimer);

    LOG_INFO("BonDriverNetwork初期化完了");
}

BonDriverNetwork::~BonDriverNetwork()
{
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

    qDebug() << "TSストリーム受信開始";
    m_tsReceiveTimer->start();
}

void BonDriverNetwork::stopReceiving()
{
    qDebug() << "TSストリーム受信停止";
    m_tsReceiveTimer->stop();
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

    qDebug() << "サーバー切断";
    m_tsReceiveTimer->stop();
    m_isInitialized = false;
    m_isTunerOpen = false;
    emit disconnected();
}

void BonDriverNetwork::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qDebug() << "<<< データ受信:" << data.size() << "bytes";

    m_receiveBuffer.append(data);
    qDebug() << "    受信バッファ合計:" << m_receiveBuffer.size() << "bytes";

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

void BonDriverNetwork::onTsReceiveTimer()
{
    if (!isConnected() || !m_isTunerOpen)
    {
        return;
    }

    // ⚠️ ログ暴走防止：GetTsStream/GetReadyCountは一旦コメントアウト
    // サーバーからの応答が無い状態での連続送信を防ぐ

    // 超高速連続TSストリーム取得（最大パフォーマンス）
    static bool hasLogged = false;
    if (!hasLogged)
    {
        LOG_INFO("🚀 連続大量取得開始 - タイマー停止してノンストップ受信");
        hasLogged = true;
        // タイマーを停止して連続受信モードに切り替え
        m_tsReceiveTimer->stop();
        // 連続受信を別スレッドで開始
        QTimer::singleShot(0, this, &BonDriverNetwork::continuousReceive);
        return;
    }
}

// 連続大量TSストリーム受信（最大パフォーマンス実現）
void BonDriverNetwork::continuousReceive()
{
    if (!isConnected() || !m_isTunerOpen || !m_isTsStreamActive) {
        // 接続切れや停止時は1秒後に再チェック
        QTimer::singleShot(1000, this, &BonDriverNetwork::continuousReceive);
        return;
    }
    
    static int loopCount = 0;
    loopCount++;
    
    // 連続大量取得（1回で50回のコマンド実行）
    for (int i = 0; i < 50; i++) {
        // GetReadyCountでバッファ確認
        if (!sendCommand(eGetReadyCount)) {
            break;
        }
        // GetTsStreamでTSデータ取得
        if (!sendCommand(eGetTsStream)) {
            break;
        }
    }
    
    // 統計ログ（削減）
    if (loopCount % 20 == 0) {
        LOG_INFO(QString("🚀 連続大量受信ループ #%1: 1回で100コマンド実行").arg(loopCount));
    }
    
    // 即座に次の連続受信をスケジュール（ノンストップ）
    QTimer::singleShot(1, this, &BonDriverNetwork::continuousReceive);
}

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
    
    // 簡素化されたデータ処理（無限ループ防止）
    int processedBytes = 0;
    const int maxProcessBytes = 188 * 10; // 最大10パケット/回
    
    while (m_receiveBuffer.size() > 0 && processedBytes < maxProcessBytes)
    {
        // BonDriverコマンドレスポンス優先処理
        if (m_receiveBuffer.size() >= 8 && static_cast<uint8_t>(m_receiveBuffer[0]) == 0xff)
        {
            if (!processCommandResponse()) {
                // コマンド処理失敗時は1バイト破棄
                m_receiveBuffer.remove(0, 1);
                processedBytes++;
            }
            continue;
        }
        
        // TSストリーム処理（高速化）
        if (m_receiveBuffer.size() >= 188) {
            // 188バイト単位で一括処理
            int tsPackets = m_receiveBuffer.size() / 188;
            int processPackets = qMin(tsPackets, 5); // 最大5パケット/回
            
            for (int i = 0; i < processPackets; i++) {
                QByteArray tsPacket = m_receiveBuffer.left(188);
                
                // 🔧 TSパケット同期バイト(0x47)チェック
                if (tsPacket.size() >= 1 && static_cast<uint8_t>(tsPacket[0]) != 0x47) {
                    // 0x47を探して同期を取る
                    int syncPos = m_receiveBuffer.indexOf(0x47);
                    if (syncPos > 0) {
                        LOG_WARNING(QString("⚠️ TS同期エラー: %1バイト破棄して0x47で再同期").arg(syncPos));
                        m_receiveBuffer.remove(0, syncPos);
                        continue; // 次のループで再処理
                    } else {
                        LOG_WARNING("⚠️ TS同期バイト(0x47)が見つかりません - バッファクリア");
                        m_receiveBuffer.clear();
                        break;
                    }
                }
                
                m_receiveBuffer.remove(0, 188);
                processedBytes += 188;
                
                static int tsDataCount = 0;
                tsDataCount++;
                
                // 従来のシグナル発行（既存機能維持）
                emit tsDataReceived(tsPacket);
                
                // TSストリーム受信完了（シンプル版）
                
                // ログ出力制限（調査用：最初の10個、その後は2000個ごと）
                if (tsDataCount <= 10 || tsDataCount % 2000 == 0) {
                    LOG_INFO(QString("📺 TSパケット #%1 (バッファ残: %2 KB)")
                             .arg(tsDataCount).arg(m_receiveBuffer.size() / 1024));
                }
            }
            continue;
        }
        
        // 不完全データまたは不正データの処理
        if (m_receiveBuffer.size() < 188) {
            // 188バイト未満の場合は次回まで待機
            break;
        } else {
            // 不正データを1バイト破棄
            m_receiveBuffer.remove(0, 1);
            processedBytes++;
        }
    }
}

bool BonDriverNetwork::processCommandResponse()
{
    if (m_receiveBuffer.size() < 8)
    {
        return false; // ヘッダーが不完全
    }

    uint8_t responseCmd = static_cast<uint8_t>(m_receiveBuffer[1]);
    uint8_t responseSize = static_cast<uint8_t>(m_receiveBuffer[7]);

    int totalPacketSize = 8 + responseSize; // ヘッダー8バイト + データ

    if (m_receiveBuffer.size() < totalPacketSize)
    {
        return false; // まだ完全なレスポンスが到着していない
    }

    QByteArray responseData = m_receiveBuffer.mid(8, responseSize);
    m_receiveBuffer.remove(0, totalPacketSize);

    // データサイズ0でも成功/失敗フラグが存在する可能性
    if (responseSize == 0 && m_receiveBuffer.size() >= 1)
    {
        QByteArray actualFlag = m_receiveBuffer.left(1);
        m_receiveBuffer.remove(0, 1);
        responseData = actualFlag;
    }

    // コマンドレスポンス処理（ログ削減）
    static int cmdResponseCount = 0;
    cmdResponseCount++;

    // ログ出力をさらに制限
    if (cmdResponseCount <= 5 || cmdResponseCount % 100 == 0)
    {
        LOG_INFO(QString("<<< コマンドレスポンス #%1: %2 bytes").arg(cmdResponseCount).arg(responseData.size()));
    }

    // TSストリーム関連レスポンス処理のみ（その他は無視してパフォーマンス向上）
    if (responseCmd == eGetTsStream && !responseData.isEmpty())
    {
        m_isTsStreamActive = true;
    }
    
    return true; // 正常処理完了
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

