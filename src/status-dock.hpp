/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QWidget>

class PluginConfig;
class SceneTracker;
class ActiveBrowserController;
class QLabel;
class QCheckBox;
class QDoubleSpinBox;
class QTimer;

/*
 * StatusDock is a small diagnostics + settings panel. It exists mainly so you
 * can VERIFY behaviour at a glance: it shows the browser source currently being
 * controlled and its live level, updating as scenes change and as you nudge.
 *
 * It also exposes the two settings: the dB step and the carry-level toggle.
 * There is no per-scene mapping UI - the active browser is detected
 * automatically.
 */
class StatusDock : public QWidget {
	Q_OBJECT

public:
	StatusDock(PluginConfig &cfg, SceneTracker &tracker, ActiveBrowserController &ctl, QWidget *parent = nullptr);

private slots:
	void refresh();
	void onCarryToggled(bool checked);
	void onStepChanged(double value);

private:
	PluginConfig &cfg_;
	SceneTracker &tracker_;
	ActiveBrowserController &ctl_;

	QLabel *sourceLabel_ = nullptr;
	QLabel *levelLabel_ = nullptr;
	QCheckBox *carryCheck_ = nullptr;
	QDoubleSpinBox *stepSpin_ = nullptr;
	QTimer *timer_ = nullptr;
};
