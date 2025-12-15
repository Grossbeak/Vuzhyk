#pragma once

#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QFileDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QKeySequenceEdit>
#include <QMap>
#include <QKeySequence>
#include <QCheckBox>

class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    QMap<QString, QKeySequence> getShortcuts() const;

signals:
    void fontSizeChanged(int size);
    void interpreterChanged();
    void showStatusMessage(const QString &message, int timeoutMs = 3000);
    void closeRequested();
    void shortcutChanged(const QString &actionName, const QKeySequence &sequence);
    void showCloseButtonOnTabsChanged(bool show);

private slots:
    void onCategoryChanged(int index);
    void onFontSizeChanged(int size);
    void onInterpreterTypeChanged();
    void onBrowsePythonPath();
    void refreshInstalledPackages();
    void onShortcutChanged(int row);
    void onResetShortcut(int row);

private:
    void setupUi();
    void loadInterpreterSettings();
    void saveInterpreterSettings();
    void loadShortcutSettings();
    void saveShortcutSettings();
    void setupShortcutsPage();
    QString resolvePythonExecutable() const;
    bool runPythonProcess(const QStringList &arguments, QByteArray &stdOutput, QByteArray &stdError, bool showErrors = true);
    void updatePackageUiEnabled();
    void ensurePackagesInitialized();
    void schedulePackagesRefresh();
    bool isInterpreterPageVisible() const;
    void populateInstalledPackagesList(const QByteArray &jsonData);
    
    QListWidget *m_categoryList { nullptr };
    QStackedWidget *m_settingsStack { nullptr };
    
    // Interface section
    QWidget *m_interfacePage { nullptr };
    QSpinBox *m_fontSizeSpinBox { nullptr };
    QCheckBox *m_closeTabWithMiddleButtonCheckBox { nullptr };
    QCheckBox *m_showCloseButtonOnTabsCheckBox { nullptr };
    
    // Interpreter section
    QWidget *m_interpreterPage { nullptr };
    QButtonGroup *m_interpreterTypeGroup { nullptr };
    QRadioButton *m_embeddedRadio { nullptr };
    QRadioButton *m_systemRadio { nullptr };
    QRadioButton *m_customRadio { nullptr };
    QLineEdit *m_pythonPathEdit { nullptr };
    QPushButton *m_browseButton { nullptr };
    
    QListWidget *m_installedPackagesList { nullptr };
    QPushButton *m_refreshPackagesButton { nullptr };
    
    // Shortcuts section
    QWidget *m_shortcutsPage { nullptr };
    QTableWidget *m_shortcutsTable { nullptr };
    QMap<QString, QKeySequence> m_defaultShortcuts; // Дефолтные горячие клавиши
    QMap<QString, QKeySequence> m_currentShortcuts; // Текущие горячие клавиши
    
    // File associations section
    QWidget *m_associationsPage { nullptr };
    QCheckBox *m_pythonAssociationCheckBox { nullptr };
    
    bool m_packagesInitialized { false };
    bool m_packagesNeedsRefresh { false };
    
    void setupAssociationsPage();
    void onPythonAssociationChanged(bool checked);
    bool setFileAssociation(const QString &extension, const QString &progId, const QString &description, const QString &command);
    bool removeFileAssociation(const QString &extension);
    QString getApplicationPath() const;
    QString extractIconFromResources() const;
    
    friend class MainWindow; // Для доступа к m_fontSizeSpinBox
};

