#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QMessageBox>
#include <exception>

#include "ui/MainWindow.h"
#include "utils/Logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // アプリケーション情報設定
    app.setApplicationName("TVTest Android");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("TVTest Team");
    app.setOrganizationDomain("tvtest.android");
    
    // 【ログ統一】Qt標準ログを完全無効化（ファイルログのみ使用）
    QLoggingCategory::setFilterRules("*=false");
    
    // Qt標準ログを完全に無効化するカスタムハンドラー
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        // 何もしない = Qt標準ログを完全無効化
        Q_UNUSED(type)
        Q_UNUSED(context)
        Q_UNUSED(msg)
    });
    
    // スタイル設定（Androidライクな外観）
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // アプリケーションディレクトリ確認
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    
    // ログシステム初期化
    try {
        if (!Logger::instance()->initialize()) {
            QMessageBox::critical(nullptr, "エラー", "ログファイルの作成に失敗しました。");
            return -1;
        }
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "エラー", QString("ログシステム初期化エラー: %1").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "エラー", "ログシステム初期化で未知のエラーが発生しました。");
        return -1;
    }
    
    LOG_INFO("=== TVTest Android 起動 ===");
    LOG_INFO(QString("アプリケーションパス: %1").arg(appDataPath));
    LOG_INFO(QString("Qt バージョン: %1").arg(qVersion()));
    LOG_INFO(QString("ログファイル: %1").arg(Logger::instance()->getLogFilePath()));
    
    // メインウィンドウ作成・表示
    MainWindow window;
    window.show();
    
    // イベントループ開始
    int result = app.exec();
    
    LOG_INFO("=== TVTest Android 終了 ===");
    Logger::instance()->flush();
    
    return result;
}