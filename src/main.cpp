#include <QApplication>
#include <QIcon>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMessageBox>
#include "MainWindow.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int main(int argc, char *argv[]) {
#ifdef _DEBUG
    // Включаем обнаружение утечек памяти
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    // Примечание: не переопределяем 'new', так как это конфликтует с Qt
    // Утечки будут показаны, но без точного указания файла и строки для Qt кода
#endif
    QApplication app(argc, argv);
    QApplication::setApplicationName("Vuzhyk");
    QApplication::setOrganizationName("Vuzhyk");
    QApplication::setWindowIcon(QIcon(":/icons/icons/logo/vuzhyk.ico"));

    // Проверяем, не запущен ли уже экземпляр приложения
    QString serverName = "VuzhykIDE_SingleInstance";
    QLocalSocket socket;
    socket.connectToServer(serverName);
    
    // Если подключение успешно, значит уже есть запущенный экземпляр
    if (socket.waitForConnected(500)) {
        // Собираем пути к файлам для открытия
        QStringList filePaths;
        
        // Обрабатываем аргументы командной строки (файлы для открытия)
#ifdef Q_OS_WIN
        int argCount;
        LPWSTR *argList = CommandLineToArgvW(GetCommandLineW(), &argCount);
        if (argList && argCount > 1) {
            for (int i = 1; i < argCount; ++i) {
                QString filePath = QString::fromWCharArray(argList[i]);
                filePath = filePath.trimmed();
                if (filePath.startsWith('"') && filePath.endsWith('"')) {
                    filePath = filePath.mid(1, filePath.length() - 2);
                }
                filePath = QDir::toNativeSeparators(QDir::cleanPath(filePath));
                if (!filePath.isEmpty() && (filePath.contains('/') || filePath.contains('\\') || filePath.endsWith(".py"))) {
                    filePaths.append(filePath);
                }
            }
            LocalFree(argList);
        }
#else
        QCommandLineParser parser;
        parser.addPositionalArgument("file", QCoreApplication::translate("main", "File to open"));
        parser.process(app);
        QStringList positionalArgs = parser.positionalArguments();
        for (const QString &arg : positionalArgs) {
            QString cleanPath = arg.trimmed();
            if (cleanPath.startsWith('"') && cleanPath.endsWith('"')) {
                cleanPath = cleanPath.mid(1, cleanPath.length() - 2);
            }
            cleanPath = QDir::toNativeSeparators(QDir::cleanPath(cleanPath));
            if (!cleanPath.isEmpty()) {
                filePaths.append(cleanPath);
            }
        }
#endif
        
        // Отправляем пути к файлам в существующий экземпляр
        if (!filePaths.isEmpty()) {
            QByteArray data = filePaths.join('\n').toUtf8();
            socket.write(data);
            socket.waitForBytesWritten(1000);
        }
        socket.disconnectFromServer();
        return 0; // Завершаем новый экземпляр
    }
    
    // Если подключение не удалось, значит это первый экземпляр - продолжаем запуск
    QCommandLineParser parser;
    parser.setApplicationDescription("Vuzhyk IDE");
    QCommandLineOption themeOption(QStringList() << "t" << "theme",
                                   QCoreApplication::translate("main", "Theme: light or dark"),
                                   QCoreApplication::translate("main", "theme"));
    parser.addOption(themeOption);
    parser.addPositionalArgument("file", QCoreApplication::translate("main", "File to open"));
    parser.process(app);

    QString theme = parser.value(themeOption);
    
    // Если тема не указана в аргументах, читаем из настроек
    if (theme.isEmpty()) {
        QSettings settings;
        theme = settings.value("theme", "light").toString();
    }

    MainWindow w(theme);
    
    // Обрабатываем аргументы командной строки (файлы для открытия)
    // Используем Windows API для правильной обработки аргументов (как в 7zip)
#ifdef Q_OS_WIN
    int argCount;
    LPWSTR *argList = CommandLineToArgvW(GetCommandLineW(), &argCount);
    if (argList && argCount > 1) {
        // Пропускаем первый аргумент (путь к exe) и обрабатываем остальные
        for (int i = 1; i < argCount; ++i) {
            QString filePath = QString::fromWCharArray(argList[i]);
            
            // Удаляем кавычки, если они есть (Windows может передавать пути в кавычках)
            filePath = filePath.trimmed();
            if (filePath.startsWith('"') && filePath.endsWith('"')) {
                filePath = filePath.mid(1, filePath.length() - 2);
            }
            
            // Нормализуем путь
            filePath = QDir::toNativeSeparators(QDir::cleanPath(filePath));
            
            // Проверяем, что это путь к файлу
            if (!filePath.isEmpty()) {
                QFileInfo fileInfo(filePath);
                if (fileInfo.exists() && fileInfo.isFile()) {
                    w.openFileFromPath(fileInfo.absoluteFilePath());
                } else {
                    // Пытаемся открыть даже если файл не найден (может быть относительный путь)
                    // Но только если путь выглядит как путь к файлу
                    if (filePath.contains('/') || filePath.contains('\\') || filePath.endsWith(".py")) {
                        w.openFileFromPath(filePath);
                    }
                }
            }
        }
        LocalFree(argList);
    }
#else
    // Для не-Windows используем стандартный способ Qt
    QStringList positionalArgs = parser.positionalArguments();
    for (const QString &arg : positionalArgs) {
        QString cleanPath = arg.trimmed();
        if (cleanPath.startsWith('"') && cleanPath.endsWith('"')) {
            cleanPath = cleanPath.mid(1, cleanPath.length() - 2);
        }
        cleanPath = QDir::toNativeSeparators(QDir::cleanPath(cleanPath));
        QFileInfo fileInfo(cleanPath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            w.openFileFromPath(fileInfo.absoluteFilePath());
        }
    }
#endif
    
    w.show();
    return app.exec();
}


