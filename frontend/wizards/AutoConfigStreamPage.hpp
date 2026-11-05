#pragma once

#include <QWizardPage>
#include <QPointer>

class QAction;
class PenguinAccountSetupRequest;

class Auth;
class Ui_AutoConfigStreamPage;

class AutoConfigStreamPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	enum class Section : int {
		Connect,
		StreamKey,
	};

	std::shared_ptr<Auth> auth;

	std::unique_ptr<Ui_AutoConfigStreamPage> ui;
	QString lastService;
	bool ready = false;
	QPointer<PenguinAccountSetupRequest> accountSetup;
	QString setupService, accountServer, accountLabel;
	bool accountConnected = false, manualKey = false;
	bool bandwidthLocked = false, previousBandwidthTest = false;
	QString PenguinProvider() const;
	QAction *PenguinAccountAction() const;


	void LoadServices(bool showAll);
	inline bool IsCustomService() const;

public:
	AutoConfigStreamPage(QWidget *parent = nullptr);
	~AutoConfigStreamPage();

	virtual bool isComplete() const override;
	virtual int nextId() const override;
	virtual bool validatePage() override;

	void OnAuthConnected();
	void OnOAuthStreamKeyConnected();

public slots:
	void on_show_clicked();
	void on_connectAccount_clicked();
	void on_disconnectAccount_clicked();
	void on_useStreamKey_clicked();
	void on_preferHardware_clicked();
	void ServiceChanged();
	void UpdateKeyLink();
	void UpdateMoreInfoLink();
	void UpdateServerList();
	void UpdateCompleted();

	void reset_service_ui_fields(std::string &service);
};
