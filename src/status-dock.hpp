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
class QCheckBox;
class QWidget;
class QVBoxLayout;
class QDoubleSpinBox;
class QScrollArea;
class QPushButton;
class QTimer;

/*
 * StatusDock shows what's being controlled and its live level, and exposes the
 * settings: the dB step, a checklist of browser sources to control (empty =
 * auto top-most on the live scene), and the optional DCA-mode toggle.
 */
class StatusDock : public QWidget {
	Q_OBJECT

public:
	StatusDock(PluginConfig &cfg, SceneTracker &tracker, ActiveBrowser &ctl, QWidget *parent = nullptr);

private slots:
	void refresh();
	void onStepChanged(double value);
	void onDcaToggled(bool checked);
	void repopulateSelection();
	void clearSelection();

private:
	void updateEnabledState();

	PluginConfig &cfg_;
	SceneTracker &tracker_;
	ActiveBrowser &ctl_;

	QLabel *headerLabel_ = nullptr;
	QLabel *bodyLabel_ = nullptr;
	QCheckBox *dcaCheck_ = nullptr;
	QLabel *selectionLabel_ = nullptr;
	QScrollArea *selectionScroll_ = nullptr;
	QWidget *selectionBody_ = nullptr;
	QVBoxLayout *selectionLayout_ = nullptr;
	QPushButton *refreshBtn_ = nullptr;
	QPushButton *clearBtn_ = nullptr;
	QDoubleSpinBox *stepSpin_ = nullptr;
	QTimer *timer_ = nullptr;
};
