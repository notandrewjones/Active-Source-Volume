/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "status-dock.hpp"
#include "config.hpp"
#include "scene-tracker.hpp"
#include "active-browser.hpp"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

StatusDock::StatusDock(PluginConfig &cfg, SceneTracker &tracker, ActiveBrowserController &ctl,
		       QWidget *parent)
	: QWidget(parent), cfg_(cfg), tracker_(tracker), ctl_(ctl)
{
	setObjectName("activeSourceVolumeDock");

	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(8, 8, 8, 8);
	outer->setSpacing(8);

	// --- live status ---
	sourceLabel_ = new QLabel(tr("Active browser: —"), this);
	sourceLabel_->setWordWrap(true);
	QFont bold = sourceLabel_->font();
	bold.setBold(true);
	sourceLabel_->setFont(bold);
	outer->addWidget(sourceLabel_);

	levelLabel_ = new QLabel(tr("Level: —"), this);
	outer->addWidget(levelLabel_);

	auto *sep = new QFrame(this);
	sep->setFrameShape(QFrame::HLine);
	sep->setFrameShadow(QFrame::Sunken);
	outer->addWidget(sep);

	// --- settings ---
	auto *form = new QFormLayout();

	carryCheck_ = new QCheckBox(tr("Carry level across scene changes"), this);
	carryCheck_->setChecked(cfg_.carry_level());
	connect(carryCheck_, &QCheckBox::toggled, this, &StatusDock::onCarryToggled);
	form->addRow(carryCheck_);

	stepSpin_ = new QDoubleSpinBox(this);
	stepSpin_->setRange(0.5, 24.0);
	stepSpin_->setSingleStep(0.5);
	stepSpin_->setDecimals(1);
	stepSpin_->setSuffix(tr(" dB"));
	stepSpin_->setValue(cfg_.nudge_step_db());
	connect(stepSpin_, &QDoubleSpinBox::valueChanged, this, &StatusDock::onStepChanged);
	form->addRow(tr("Hotkey step:"), stepSpin_);

	outer->addLayout(form);

	auto *reresolve = new QPushButton(tr("Re-resolve active browser"), this);
	connect(reresolve, &QPushButton::clicked, this, [this]() { tracker_.resolve_current(); });
	outer->addWidget(reresolve);

	outer->addStretch();

	// Poll live state on the UI thread (controller is mutex-guarded).
	timer_ = new QTimer(this);
	timer_->setInterval(250);
	connect(timer_, &QTimer::timeout, this, &StatusDock::refresh);
	timer_->start();

	refresh();
}

void StatusDock::refresh()
{
	std::string name = ctl_.active_name();
	if (name.empty()) {
		sourceLabel_->setText(tr("Active browser: — (no browser in live scene)"));
		levelLabel_->setText(tr("Level: —"));
		return;
	}

	sourceLabel_->setText(tr("Active browser: %1").arg(QString::fromStdString(name)));

	float db = 0.0f;
	bool muted = false;
	bool haveDb = ctl_.get_db(db);
	ctl_.get_mute(muted);

	QString level;
	if (!haveDb)
		level = tr("Level: —");
	else if (db <= ActiveBrowserController::kFloorDb)
		level = tr("Level: -∞ dB");
	else
		level = tr("Level: %1 dB").arg(db, 0, 'f', 1);
	if (muted)
		level += tr("  (MUTED)");

	levelLabel_->setText(level);
}

void StatusDock::onCarryToggled(bool checked)
{
	cfg_.set_carry_level(checked);
	ctl_.set_carry_level(checked);
	cfg_.save();
}

void StatusDock::onStepChanged(double value)
{
	cfg_.set_nudge_step_db((float)value);
	cfg_.save();
}
