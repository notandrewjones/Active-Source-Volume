/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QWidget>

class PluginConfig;
class SceneTracker;
class ActiveBrowser;
class QLabel;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QTimer;

/*
 * StatusDock shows what's being controlled and its live level, and exposes the
 * settings: the dB step, the controlled-source picker (Auto or a pinned
 * override), and the optional DCA-mode toggle (control every browser source at
 * once). It reads state directly from ActiveBrowser / browser_dca.
 */
class StatusDock : public QWidget {
	Q_OBJECT

public:
	StatusDock(PluginConfig &cfg, SceneTracker &tracker, ActiveBrowser &ctl, QWidget *parent = nullptr);

private slots:
	void refresh();
	void onStepChanged(double value);
	void onOverrideChanged(const QString &text);
	void onDcaToggled(bool checked);

private:
	void repopulateOverride();
	void updateEnabledState();

	PluginConfig &cfg_;
	SceneTracker &tracker_;
	ActiveBrowser &ctl_;

	QLabel *headerLabel_ = nullptr;
	QLabel *bodyLabel_ = nullptr;
	QCheckBox *dcaCheck_ = nullptr;
	QComboBox *overrideCombo_ = nullptr;
	QLabel *overrideLabel_ = nullptr;
	QDoubleSpinBox *stepSpin_ = nullptr;
	QTimer *timer_ = nullptr;
	bool populating_ = false;
};
