#include "SettingsWidget.h"
#include <QSettings>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QKeySequence>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#endif

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    
    // Загружаем сохраненный размер шрифта
    QSettings settings;
    int fontSize = settings.value("editor/fontSize", 10).toInt();
    m_fontSizeSpinBox->setValue(fontSize);
    
    // Загружаем настройки интерпретатора
    loadInterpreterSettings();
    
    // Загружаем настройки горячих клавиш
    loadShortcutSettings();
    
    // Инициализируем состояние раздела библиотек (без тяжелых операций)
    updatePackageUiEnabled();
}

void SettingsWidget::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Контейнер для правой части (настройки + кнопка закрытия)
    auto *rightContainer = new QWidget(this);
    auto *rightContainerLayout = new QVBoxLayout(rightContainer);
    rightContainerLayout->setContentsMargins(0, 0, 0, 0);
    rightContainerLayout->setSpacing(0);
    
    // Кнопка закрытия в правом верхнем углу
    auto *closeButton = new QPushButton(tr("×"), this);
    closeButton->setFixedSize(30, 30);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "    color: #666;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(0, 0, 0, 0.1);"
        "    color: #000;"
        "}"
    );
    connect(closeButton, &QPushButton::clicked, this, &SettingsWidget::closeRequested);
    
    // Контейнер для кнопки закрытия (выравнивание по правому краю)
    auto *closeButtonContainer = new QWidget(rightContainer);
    auto *closeButtonLayout = new QHBoxLayout(closeButtonContainer);
    closeButtonLayout->setContentsMargins(0, 0, 5, 5);
    closeButtonLayout->setSpacing(0);
    closeButtonLayout->addStretch();
    closeButtonLayout->addWidget(closeButton);
    
    rightContainerLayout->addWidget(closeButtonContainer);
    
    // Левая панель с категориями
    m_categoryList = new QListWidget(this);
    m_categoryList->setFixedWidth(150);
    m_categoryList->addItem(tr("Интерфейс"));
    m_categoryList->addItem(tr("Интерпретатор"));
    m_categoryList->addItem(tr("Горячие клавиши"));
    m_categoryList->addItem(tr("Ассоциации"));
    m_categoryList->setCurrentRow(0);
    connect(m_categoryList, &QListWidget::currentRowChanged, this, &SettingsWidget::onCategoryChanged);
    
    // Правая панель со страницами настроек
    m_settingsStack = new QStackedWidget(rightContainer);
    
    // Страница "Интерфейс"
    m_interfacePage = new QWidget(m_settingsStack);
    auto *interfaceLayout = new QVBoxLayout(m_interfacePage);
    interfaceLayout->setContentsMargins(20, 20, 20, 20);
    interfaceLayout->setSpacing(15);
    
    // Размер текста в редакторе
    auto *fontSizeLabel = new QLabel(tr("Размер текста в редакторе:"), m_interfacePage);
    m_fontSizeSpinBox = new QSpinBox(m_interfacePage);
    m_fontSizeSpinBox->setRange(6, 72);
    m_fontSizeSpinBox->setSuffix(tr(" пт"));
    connect(m_fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &SettingsWidget::onFontSizeChanged);
    
    auto *fontSizeLayout = new QHBoxLayout();
    fontSizeLayout->addWidget(fontSizeLabel);
    fontSizeLayout->addWidget(m_fontSizeSpinBox);
    fontSizeLayout->addStretch();
    
    interfaceLayout->addLayout(fontSizeLayout);
    
    // Закрытие вкладок средней кнопкой мыши
    m_closeTabWithMiddleButtonCheckBox = new QCheckBox(tr("Закрытие вкладок средней кнопкой мыши"), m_interfacePage);
    QSettings settings;
    bool closeWithMiddleButton = settings.value("interface/closeTabWithMiddleButton", false).toBool();
    m_closeTabWithMiddleButtonCheckBox->setChecked(closeWithMiddleButton);
    connect(m_closeTabWithMiddleButtonCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        QSettings settings;
        settings.setValue("interface/closeTabWithMiddleButton", checked);
    });
    interfaceLayout->addWidget(m_closeTabWithMiddleButtonCheckBox);
    
    // Отображать кнопку закрытия на вкладке
    m_showCloseButtonOnTabsCheckBox = new QCheckBox(tr("Отображать кнопку закрытия на вкладке"), m_interfacePage);
    bool showCloseButton = settings.value("interface/showCloseButtonOnTabs", false).toBool();
    m_showCloseButtonOnTabsCheckBox->setChecked(showCloseButton);
    connect(m_showCloseButtonOnTabsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        QSettings settings;
        settings.setValue("interface/showCloseButtonOnTabs", checked);
        emit showCloseButtonOnTabsChanged(checked);
    });
    interfaceLayout->addWidget(m_showCloseButtonOnTabsCheckBox);
    
    interfaceLayout->addStretch();
    
    m_settingsStack->addWidget(m_interfacePage);
    
    // Страница "Интерпретатор"
    m_interpreterPage = new QWidget(m_settingsStack);
    auto *interpreterLayout = new QVBoxLayout(m_interpreterPage);
    interpreterLayout->setContentsMargins(20, 20, 20, 20);
    interpreterLayout->setSpacing(15);
    
    // Группа радио-кнопок для выбора типа интерпретатора
    m_interpreterTypeGroup = new QButtonGroup(m_interpreterPage);
    
    m_embeddedRadio = new QRadioButton(tr("Встроенный в IDE"), m_interpreterPage);
    m_systemRadio = new QRadioButton(tr("Системный"), m_interpreterPage);
    m_customRadio = new QRadioButton(tr("Выбрать путь"), m_interpreterPage);
    
    m_interpreterTypeGroup->addButton(m_embeddedRadio, 0);
    m_interpreterTypeGroup->addButton(m_systemRadio, 1);
    m_interpreterTypeGroup->addButton(m_customRadio, 2);
    
    connect(m_interpreterTypeGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &SettingsWidget::onInterpreterTypeChanged);
    
    interpreterLayout->addWidget(m_embeddedRadio);
    interpreterLayout->addWidget(m_systemRadio);
    interpreterLayout->addWidget(m_customRadio);
    
    // Поле для пути к Python (только для "Выбрать путь")
    auto *pathLayout = new QHBoxLayout();
    auto *pathLabel = new QLabel(tr("Путь к интерпретатору:"), m_interpreterPage);
    m_pythonPathEdit = new QLineEdit(m_interpreterPage);
    m_pythonPathEdit->setPlaceholderText(tr("Выберите путь к python.exe"));
    m_pythonPathEdit->setEnabled(false);
    
    m_browseButton = new QPushButton(tr("Обзор..."), m_interpreterPage);
    m_browseButton->setEnabled(false);
    connect(m_browseButton, &QPushButton::clicked, this, &SettingsWidget::onBrowsePythonPath);
    
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_pythonPathEdit, 1);
    pathLayout->addWidget(m_browseButton);
    
    interpreterLayout->addLayout(pathLayout);
    
    // Список установленных библиотек
    auto *packagesHeaderLayout = new QHBoxLayout();
    packagesHeaderLayout->addStretch();
    m_refreshPackagesButton = new QPushButton(tr("Обновить список"), m_interpreterPage);
    packagesHeaderLayout->addWidget(m_refreshPackagesButton);
    interpreterLayout->addLayout(packagesHeaderLayout);
    
    m_installedPackagesList = new QListWidget(m_interpreterPage);
    m_installedPackagesList->setUniformItemSizes(true);
    interpreterLayout->addWidget(m_installedPackagesList);
    
    connect(m_refreshPackagesButton, &QPushButton::clicked,
            this, &SettingsWidget::refreshInstalledPackages);
    interpreterLayout->addStretch();
    
    m_settingsStack->addWidget(m_interpreterPage);
    
    // Страница "Горячие клавиши"
    setupShortcutsPage();
    m_settingsStack->addWidget(m_shortcutsPage);
    
    // Страница "Ассоциации"
    setupAssociationsPage();
    m_settingsStack->addWidget(m_associationsPage);
    
    rightContainerLayout->addWidget(m_settingsStack, 1);
    
    mainLayout->addWidget(m_categoryList);
    mainLayout->addWidget(rightContainer, 1);
}

void SettingsWidget::onCategoryChanged(int index)
{
    if (index >= 0 && index < m_settingsStack->count()) {
        m_settingsStack->setCurrentIndex(index);
        
        if (m_settingsStack->widget(index) == m_interpreterPage) {
            ensurePackagesInitialized();
        }
    }
}

QMap<QString, QKeySequence> SettingsWidget::getShortcuts() const
{
    return m_currentShortcuts;
}

void SettingsWidget::onFontSizeChanged(int size)
{
    // Сохраняем настройку
    QSettings settings;
    settings.setValue("editor/fontSize", size);
    
    // Отправляем сигнал об изменении
    emit fontSizeChanged(size);
}

void SettingsWidget::loadInterpreterSettings()
{
    QSettings settings;
    settings.beginGroup("runtime");
    
    // Загружаем тип интерпретатора (0=встроенный, 1=системный, 2=пользовательский)
    int type = settings.value("interpreterType", 0).toInt();
    
    // Загружаем путь к Python
    QString path = settings.value("pythonPath").toString();
    
    settings.endGroup();
    
    // Устанавливаем выбранную радио-кнопку
    if (type == 0) {
        m_embeddedRadio->setChecked(true);
    } else if (type == 1) {
        m_systemRadio->setChecked(true);
    } else {
        m_customRadio->setChecked(true);
    }
    
    // Устанавливаем путь
    m_pythonPathEdit->setText(path);
    
    // Включаем/выключаем поле пути в зависимости от выбора
    onInterpreterTypeChanged();
}

void SettingsWidget::saveInterpreterSettings()
{
    QSettings settings;
    settings.beginGroup("runtime");
    
    // Сохраняем тип интерпретатора
    int type = 0;
    if (m_embeddedRadio->isChecked()) {
        type = 0;
    } else if (m_systemRadio->isChecked()) {
        type = 1;
    } else {
        type = 2;
    }
    settings.setValue("interpreterType", type);
    
    // Сохраняем путь к Python (только если выбран пользовательский)
    if (type == 2) {
        settings.setValue("pythonPath", m_pythonPathEdit->text());
    }
    
    settings.endGroup();
}

void SettingsWidget::onInterpreterTypeChanged()
{
    // Включаем/выключаем поле пути и кнопку "Обзор" в зависимости от выбора
    bool isCustom = m_customRadio->isChecked();
    m_pythonPathEdit->setEnabled(isCustom);
    m_browseButton->setEnabled(isCustom);
    
    // Сохраняем настройки
    saveInterpreterSettings();
    
    // Отправляем сигнал об изменении
    emit interpreterChanged();
    
    updatePackageUiEnabled();
    schedulePackagesRefresh();
}

void SettingsWidget::onBrowsePythonPath()
{
    QString startDir;
    if (!m_pythonPathEdit->text().isEmpty()) {
        QFileInfo info(m_pythonPathEdit->text());
        startDir = info.absolutePath();
    } else {
        // Пытаемся найти Python в стандартных местах
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString embeddedPython = QDir(appDir).filePath("python/python.exe");
        if (QFile::exists(embeddedPython)) {
            startDir = QFileInfo(embeddedPython).absolutePath();
        } else {
            startDir = QDir::homePath();
        }
    }
    
    QString path = QFileDialog::getOpenFileName(
        this, tr("Выбрать интерпретатор Python"), startDir,
        tr("Исполняемые (*.exe);;Все файлы (*.*)"));
    
    if (!path.isEmpty()) {
        m_pythonPathEdit->setText(path);
        saveInterpreterSettings();
        emit interpreterChanged();
        updatePackageUiEnabled();
        schedulePackagesRefresh();
    }
}

void SettingsWidget::refreshInstalledPackages()
{
    m_packagesInitialized = true;
    m_packagesNeedsRefresh = false;
    
    const QString python = resolvePythonExecutable();
    if (python.isEmpty() || !QFile::exists(python)) {
        updatePackageUiEnabled();
        return;
    }
    
    m_installedPackagesList->clear();
    
    QByteArray stdOut;
    QByteArray stdErr;
    if (!runPythonProcess({"-m", "pip", "list", "--format=json"}, stdOut, stdErr)) {
        return;
    }
    
    populateInstalledPackagesList(stdOut);
}

void SettingsWidget::ensurePackagesInitialized()
{
    if (!m_packagesInitialized || m_packagesNeedsRefresh) {
        refreshInstalledPackages();
    }
}

void SettingsWidget::schedulePackagesRefresh()
{
    if (isInterpreterPageVisible()) {
        refreshInstalledPackages();
    } else {
        m_packagesNeedsRefresh = true;
    }
}

bool SettingsWidget::isInterpreterPageVisible() const
{
    return m_settingsStack && m_settingsStack->currentWidget() == m_interpreterPage;
}

QString SettingsWidget::resolvePythonExecutable() const
{
    QSettings settings;
    settings.beginGroup("runtime");
    const int type = settings.value("interpreterType", 0).toInt();
    const QString customPath = settings.value("pythonPath").toString();
    settings.endGroup();
    
    if (type == 2) { // Пользовательский путь
        if (QFile::exists(customPath))
            return customPath;
        return {};
    }
    
    if (type == 0) { // Встроенный
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString embedded = QDir(appDir).filePath("python/python.exe");
        if (QFile::exists(embedded))
            return embedded;
        // fallback на системный
    }
    
    return QStringLiteral("python.exe");
}

bool SettingsWidget::runPythonProcess(const QStringList &arguments, QByteArray &stdOutput, QByteArray &stdError, bool showErrors)
{
    const QString python = resolvePythonExecutable();
    if (python.isEmpty() || !QFile::exists(python)) {
        if (showErrors) {
            QMessageBox::warning(this, tr("Python не найден"),
                                 tr("Не удалось найти интерпретатор Python. Проверьте настройки интерпретатора."));
        }
        updatePackageUiEnabled();
        return false;
    }
    
    QProcess process;
    process.setProgram(python);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    
    process.start();
    if (!process.waitForFinished(60000)) {
        process.kill();
        if (showErrors) {
            QMessageBox::warning(this, tr("Таймаут"),
                                 tr("Команда pip выполняется слишком долго и была остановлена."));
        }
        return false;
    }
    
    stdOutput = process.readAllStandardOutput();
    stdError = process.readAllStandardError();
    
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString errorText = QString::fromUtf8(stdError).trimmed();
        if (errorText.isEmpty())
            errorText = tr("Неизвестная ошибка при выполнении команды pip.");
        if (showErrors) {
            QMessageBox::warning(this, tr("Ошибка pip"), errorText);
        }
        return false;
    }
    
    return true;
}

void SettingsWidget::updatePackageUiEnabled()
{
    const QString python = resolvePythonExecutable();
    const bool available = !python.isEmpty() && QFile::exists(python);
    
    if (m_refreshPackagesButton) m_refreshPackagesButton->setEnabled(available);
    if (m_installedPackagesList) m_installedPackagesList->setEnabled(available);
    
    if (!available) {
        if (m_installedPackagesList) m_installedPackagesList->clear();
    }
}

void SettingsWidget::populateInstalledPackagesList(const QByteArray &jsonData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        QMessageBox::warning(this, tr("Ошибка разбора"),
                             tr("Не удалось разобрать вывод pip list."));
        return;
    }
    
    const QJsonArray array = doc.array();
    QStringList packages;
    packages.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            continue;
        const QJsonObject obj = value.toObject();
        const QString name = obj.value(QStringLiteral("name")).toString();
        const QString version = obj.value(QStringLiteral("version")).toString();
        if (!name.isEmpty()) {
            if (!version.isEmpty())
                packages << QString("%1 %2").arg(name, version);
            else
                packages << name;
        }
    }
    packages.sort(Qt::CaseInsensitive);
    
    for (const QString &pkg : packages) {
        m_installedPackagesList->addItem(pkg);
    }
}

void SettingsWidget::setupShortcutsPage()
{
    m_shortcutsPage = new QWidget(m_settingsStack);
    auto *shortcutsLayout = new QVBoxLayout(m_shortcutsPage);
    shortcutsLayout->setContentsMargins(20, 20, 20, 20);
    shortcutsLayout->setSpacing(15);
    
    // Заголовок
    auto *titleLabel = new QLabel(tr("Горячие клавиши"), m_shortcutsPage);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    shortcutsLayout->addWidget(titleLabel);
    
    // Таблица горячих клавиш
    m_shortcutsTable = new QTableWidget(m_shortcutsPage);
    m_shortcutsTable->setColumnCount(3);
    m_shortcutsTable->setHorizontalHeaderLabels({tr("Действие"), tr("Горячая клавиша"), tr("")});
    m_shortcutsTable->horizontalHeader()->setStretchLastSection(false);
    m_shortcutsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_shortcutsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_shortcutsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_shortcutsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_shortcutsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_shortcutsTable->verticalHeader()->setVisible(false);
    
    // Определяем список действий с их дефолтными горячими клавишами
    struct ShortcutInfo {
        QString actionName;
        QString displayName;
        QKeySequence defaultSequence;
    };
    
    QList<ShortcutInfo> shortcuts = {
        {"new", tr("Новый файл"), QKeySequence::New},
        {"open", tr("Открыть файл"), QKeySequence::Open},
        {"save", tr("Сохранить"), QKeySequence::Save},
        {"saveAs", tr("Сохранить как..."), QKeySequence::SaveAs},
        {"saveAll", tr("Сохранить все"), QKeySequence(QStringLiteral("Ctrl+Shift+S"))},
        {"run", tr("Запустить скрипт"), QKeySequence(Qt::Key_F5)},
        {"runInTerminal", tr("Запуск в отдельном окне консоли"), QKeySequence(Qt::Key_F6)},
        {"debug", tr("Запуск с отладкой"), QKeySequence(Qt::Key_F8)},
    };
    
    m_shortcutsTable->setRowCount(shortcuts.size());
    
    for (int i = 0; i < shortcuts.size(); ++i) {
        const auto &info = shortcuts[i];
        
        // Название действия
        auto *nameItem = new QTableWidgetItem(info.displayName);
        nameItem->setData(Qt::UserRole, info.actionName); // Сохраняем имя действия
        m_shortcutsTable->setItem(i, 0, nameItem);
        
        // Редактор горячей клавиши
        QKeySequenceEdit *keyEdit = new QKeySequenceEdit(m_shortcutsPage);
        QKeySequence currentSeq = m_currentShortcuts.value(info.actionName, info.defaultSequence);
        keyEdit->setKeySequence(currentSeq);
        m_shortcutsTable->setCellWidget(i, 1, keyEdit);
        
        // Сохраняем дефолтное значение
        m_defaultShortcuts[info.actionName] = info.defaultSequence;
        
        // Кнопка сброса
        auto *resetButton = new QPushButton(tr("Сбросить"), m_shortcutsPage);
        resetButton->setProperty("row", i);
        resetButton->setProperty("actionName", info.actionName);
        connect(resetButton, &QPushButton::clicked, this, [this, resetButton]() {
            onResetShortcut(resetButton->property("row").toInt());
        });
        m_shortcutsTable->setCellWidget(i, 2, resetButton);
        
        // Подключаем сигнал изменения горячей клавиши
        connect(keyEdit, &QKeySequenceEdit::keySequenceChanged, this, [this, i, info]() {
            onShortcutChanged(i);
        });
    }
    
    shortcutsLayout->addWidget(m_shortcutsTable);
    shortcutsLayout->addStretch();
}

void SettingsWidget::loadShortcutSettings()
{
    QSettings settings;
    settings.beginGroup("shortcuts");
    
    // Загружаем сохраненные горячие клавиши
    QStringList keys = settings.allKeys();
    for (const QString &key : keys) {
        QString sequenceStr = settings.value(key).toString();
        if (!sequenceStr.isEmpty()) {
            m_currentShortcuts[key] = QKeySequence(sequenceStr);
        }
    }
    
    settings.endGroup();
}

void SettingsWidget::saveShortcutSettings()
{
    QSettings settings;
    settings.beginGroup("shortcuts");
    
    // Сохраняем все текущие горячие клавиши
    for (auto it = m_currentShortcuts.begin(); it != m_currentShortcuts.end(); ++it) {
        if (it.value() != m_defaultShortcuts.value(it.key())) {
            // Сохраняем только если отличается от дефолтного
            settings.setValue(it.key(), it.value().toString());
        } else {
            // Удаляем из настроек, если вернулось к дефолтному
            settings.remove(it.key());
        }
    }
    
    settings.endGroup();
}

void SettingsWidget::onShortcutChanged(int row)
{
    QKeySequenceEdit *keyEdit = qobject_cast<QKeySequenceEdit*>(m_shortcutsTable->cellWidget(row, 1));
    if (!keyEdit) return;
    
    QTableWidgetItem *nameItem = m_shortcutsTable->item(row, 0);
    if (!nameItem) return;
    
    QString actionName = nameItem->data(Qt::UserRole).toString();
    QKeySequence newSequence = keyEdit->keySequence();
    
    // Обновляем текущую горячую клавишу
    m_currentShortcuts[actionName] = newSequence;
    
    // Сохраняем настройки
    saveShortcutSettings();
    
    // Отправляем сигнал об изменении
    emit shortcutChanged(actionName, newSequence);
}

void SettingsWidget::onResetShortcut(int row)
{
    QTableWidgetItem *nameItem = m_shortcutsTable->item(row, 0);
    if (!nameItem) return;
    
    QString actionName = nameItem->data(Qt::UserRole).toString();
    QKeySequence defaultSeq = m_defaultShortcuts.value(actionName);
    
    // Обновляем редактор
    QKeySequenceEdit *keyEdit = qobject_cast<QKeySequenceEdit*>(m_shortcutsTable->cellWidget(row, 1));
    if (keyEdit) {
        keyEdit->setKeySequence(defaultSeq);
    }
    
    // Обновляем текущую горячую клавишу
    m_currentShortcuts[actionName] = defaultSeq;
    
    // Сохраняем настройки
    saveShortcutSettings();
    
    // Отправляем сигнал об изменении
    emit shortcutChanged(actionName, defaultSeq);
}

void SettingsWidget::setupAssociationsPage()
{
    m_associationsPage = new QWidget(m_settingsStack);
    auto *associationsLayout = new QVBoxLayout(m_associationsPage);
    associationsLayout->setContentsMargins(20, 20, 20, 20);
    associationsLayout->setSpacing(15);
    
    // Заголовок
    auto *titleLabel = new QLabel(tr("Ассоциации файлов"), m_associationsPage);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    associationsLayout->addWidget(titleLabel);
    
    // Описание
    auto *descriptionLabel = new QLabel(
        tr("Выберите типы файлов, которые будут открываться в Vuzhyk по умолчанию."),
        m_associationsPage);
    descriptionLabel->setWordWrap(true);
    associationsLayout->addWidget(descriptionLabel);
    
    // Чекбокс для Python файлов
    m_pythonAssociationCheckBox = new QCheckBox(tr("Python (.py)"), m_associationsPage);
    
    // Загружаем текущее состояние ассоциации
    QSettings settings;
    bool pythonAssociated = settings.value("associations/python", false).toBool();
    m_pythonAssociationCheckBox->setChecked(pythonAssociated);
    
    connect(m_pythonAssociationCheckBox, &QCheckBox::toggled, 
            this, &SettingsWidget::onPythonAssociationChanged);
    
    associationsLayout->addWidget(m_pythonAssociationCheckBox);
    associationsLayout->addStretch();
}

void SettingsWidget::onPythonAssociationChanged(bool checked)
{
    QSettings settings;
    settings.setValue("associations/python", checked);
    
    const QString extension = ".py";
    const QString progId = "Vuzhyk.Python";
    const QString description = tr("Файл Python");
    QString appPath = getApplicationPath();
    
    // Нормализуем путь к приложению (убираем лишние слеши)
    appPath = QDir::toNativeSeparators(QDir::cleanPath(appPath));
    
    if (checked) {
        // Устанавливаем ассоциацию
        // Формируем команду как в 7zip: "путь_к_exe" "%1"
        // Важно: путь к exe должен быть в кавычках, и %1 тоже в кавычках
        QString command = QString("\"%1\" \"%2\"").arg(appPath, "%1");
        if (setFileAssociation(extension, progId, description, command)) {
            emit showStatusMessage(
                tr("Ассоциация для .py файлов установлена. Vuzhyk появится в списке 'Открыть с помощью' после обновления проводника."), 
                5000);
        } else {
            m_pythonAssociationCheckBox->setChecked(false);
            settings.setValue("associations/python", false);
            emit showStatusMessage(tr("Не удалось установить ассоциацию. Требуются права администратора."), 5000);
        }
    } else {
        // Удаляем ассоциацию
        if (removeFileAssociation(extension)) {
            emit showStatusMessage(tr("Ассоциация для .py файлов удалена"), 3000);
        } else {
            emit showStatusMessage(tr("Не удалось удалить ассоциацию. Требуются права администратора."), 5000);
        }
    }
}

bool SettingsWidget::setFileAssociation(const QString &extension, const QString &progId, const QString &description, const QString &command)
{
#ifdef Q_OS_WIN
    // Реализация по образцу 7zip
    // Используем HKEY_CURRENT_USER\Software\Classes (не требует прав администратора)
    
    QString baseKey = QString("HKEY_CURRENT_USER\\Software\\Classes");
    QString appPath = getApplicationPath();
    
    // 1. Удаляем старую ассоциацию (если есть) - как в 7zip
    removeFileAssociation(extension);
    
    // 2. Создаем ключ расширения и связываем его с ProgID
    // .py -> Default = "Vuzhyk.Python"
    QSettings extSettings(baseKey + "\\" + extension, QSettings::NativeFormat);
    extSettings.setValue("Default", progId);
    
    // 2.1. Устанавливаем иконку для расширения .py
    // Извлекаем иконку из ресурсов во временный файл
    QString iconFilePath = extractIconFromResources();
    QString extIconPath = QString("\"%1\",0").arg(iconFilePath);
    QSettings extIconSettings(baseKey + "\\" + extension + "\\DefaultIcon", QSettings::NativeFormat);
    extIconSettings.setValue("Default", extIconPath);
    extIconSettings.sync();
    
    extSettings.sync();
    
    // 3. Создаем ProgID ключ с полной структурой (как в 7zip)
    QString progIdKey = baseKey + "\\" + progId;
    
    // 3.1. Устанавливаем описание программы
    QSettings progIdSettings(progIdKey, QSettings::NativeFormat);
    progIdSettings.setValue("Default", description);
    
    // 3.2. Устанавливаем иконку (используем ту же, что и для расширения)
    QString iconPath = QString("\"%1\",0").arg(iconFilePath);
    QSettings iconSettings(progIdKey + "\\DefaultIcon", QSettings::NativeFormat);
    iconSettings.setValue("Default", iconPath);
    iconSettings.sync();
    
    // 3.3. Создаем структуру shell\open\command (как в 7zip)
    QSettings shellSettings(progIdKey + "\\shell", QSettings::NativeFormat);
    shellSettings.setValue("Default", ""); // Пустое значение для shell
    shellSettings.sync();
    
    QSettings openSettings(progIdKey + "\\shell\\open", QSettings::NativeFormat);
    openSettings.setValue("Default", ""); // Пустое значение для open
    openSettings.sync();
    
    QSettings commandSettings(progIdKey + "\\shell\\open\\command", QSettings::NativeFormat);
    commandSettings.setValue("Default", command);
    commandSettings.sync();
    
    progIdSettings.sync();
    
    // 4. Регистрируем приложение в Applications для появления в "Открыть с помощью"
    QFileInfo appInfo(appPath);
    QString appName = appInfo.fileName();
    QString appKey = baseKey + "\\Applications\\" + appName;
    QSettings appSettings(appKey, QSettings::NativeFormat);
    appSettings.setValue("FriendlyAppName", "Vuzhyk");
    appSettings.setValue("shell/open/command/Default", command);
    appSettings.setValue("DefaultIcon/Default", iconPath);
    
    // 5. Добавляем расширение в список поддерживаемых расширений приложения
    QSettings appSupportedExtSettings(appKey + "\\SupportedTypes", QSettings::NativeFormat);
    appSupportedExtSettings.setValue(extension, "");
    appSupportedExtSettings.sync();
    
    // 6. Добавляем в OpenWithProgids для появления в "Открыть с помощью"
    QSettings openWithSettings(baseKey + "\\" + extension + "\\OpenWithProgids", QSettings::NativeFormat);
    openWithSettings.setValue(progId, "");
    openWithSettings.sync();
    
    appSettings.sync();
    
    // Уведомляем Windows об изменении ассоциаций файлов
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    
    return true;
#else
    Q_UNUSED(extension);
    Q_UNUSED(progId);
    Q_UNUSED(description);
    Q_UNUSED(command);
    return false;
#endif
}

bool SettingsWidget::removeFileAssociation(const QString &extension)
{
#ifdef Q_OS_WIN
    QString baseKey = QString("HKEY_CURRENT_USER\\Software\\Classes");
    const QString progId = "Vuzhyk.Python";
    QString appPath = getApplicationPath();
    QFileInfo appInfo(appPath);
    QString appName = appInfo.fileName();
    
    // 1. Удаляем ключ расширения полностью (как в 7zip)
    QSettings extSettings(baseKey + "\\" + extension, QSettings::NativeFormat);
    extSettings.clear(); // Удаляем весь ключ
    extSettings.sync();
    
    // 2. Удаляем ProgID ключ полностью (как в 7zip)
    QSettings progIdSettings(baseKey + "\\" + progId, QSettings::NativeFormat);
    progIdSettings.clear(); // Удаляем весь ключ
    progIdSettings.sync();
    
    // 3. Удаляем из OpenWithProgids
    QSettings openWithSettings(baseKey + "\\" + extension + "\\OpenWithProgids", QSettings::NativeFormat);
    openWithSettings.remove(progId);
    openWithSettings.sync();
    
    // 4. Удаляем расширение из списка поддерживаемых приложением
    QString appKey = baseKey + "\\Applications\\" + appName + "\\SupportedTypes";
    QSettings appSupportedExtSettings(appKey, QSettings::NativeFormat);
    appSupportedExtSettings.remove(extension);
    appSupportedExtSettings.sync();
    
    // Уведомляем Windows об изменении ассоциаций файлов
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    
    return true;
#else
    Q_UNUSED(extension);
    return false;
#endif
}

QString SettingsWidget::getApplicationPath() const
{
    return QCoreApplication::applicationFilePath();
}

QString SettingsWidget::extractIconFromResources() const
{
    // Извлекаем иконку из ресурсов во временный файл
    QString tempDir = QDir::tempPath();
    QString iconPath = QDir(tempDir).filePath("Vuzhyk_py_icon.ico");
    
    // Если файл уже существует, используем его
    if (QFile::exists(iconPath)) {
        return iconPath;
    }
    
    // Пытаемся загрузить иконку из ресурсов
    QFile resourceFile(":/icons/icons/logo/py.ico");
    if (resourceFile.open(QIODevice::ReadOnly)) {
        QByteArray iconData = resourceFile.readAll();
        resourceFile.close();
        
        // Сохраняем во временный файл
        QFile tempFile(iconPath);
        if (tempFile.open(QIODevice::WriteOnly)) {
            tempFile.write(iconData);
            tempFile.close();
            return iconPath;
        }
    }
    
    // Если не удалось извлечь из ресурсов, используем иконку из exe
    return getApplicationPath();
}

