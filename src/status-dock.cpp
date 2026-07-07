/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "status-dock.hpp"
#include "config.hpp"
#include "scene-tracker.hpp"
#include "active-browser.hpp"
#include "browser-dca.hpp"

#include <obs.h>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace {

std::vector<std::string> browser_source_names()
{
	std::vector<std::string> names;
	obs_enum_sources(
		[](void *param, obs_source_t *s) -> bool {
			auto *v = static_cast<std::vector<std::string> *>(param);
			const char *id = obs_source_get_unversioned_id(s);
			if (id && strcmp(id, "browser_source") == 0) {
				const char *n = obs_source_get_name(s);
				if (n && *n)
					v->emplace_back(n);
			}
			return true;
		},
		&names);
	std::sort(names.begin(), names.end());
	return names;
}

QString db_text(float db)
{
	if (db <= browser_dca::kFloorDb)
		return QStringLiteral("-inf dB");
	return QString::asprintf("%+.1f dB", db);
}

} // namespace

StatusDock::StatusDock(PluginConfig &cfg, SceneTracker &tracker, ActiveBrowser &ctl, QWidget *parent)
	: QWidget(parent),
	  cfg_(cfg),
	  tracker_(tracker),
	  ctl_(ctl)
{
	setObjectName("activeSourceVolumeDock");

	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(8, 8, 8, 8);
	outer->setSpacing(8);

	auto *intro = new QLabel(tr("The hotkeys shift ONE browser source at a time (so alert / donation / chat "
				    "browsers are left alone). Or enable DCA mode to shift every browser at once."),
				 this);
	intro->setWordWrap(true);
	outer->addWidget(intro);

	dcaCheck_ = new QCheckBox(tr("DCA mode: control all browser sources at once"), this);
	dcaCheck_->setChecked(cfg_.dca_mode());
	connect(dcaCheck_, &QCheckBox::toggled, this, &StatusDock::onDcaToggled);
	outer->addWidget(dcaCheck_);

	auto *sep1 = new QFrame(this);
	sep1->setFrameShape(QFrame::HLine);
	sep1->setFrameShadow(QFrame::Sunken);
	outer->addWidget(sep1);

	selectionLabel_ = new QLabel(tr("Check the browser source(s) to control. They're controlled only while on the "
					"live scene. Check none to auto-pick the top-most browser on each scene."),
				     this);
	selectionLabel_->setWordWrap(true);
	outer->addWidget(selectionLabel_);

	selectionScroll_ = new QScrollArea(this);
	selectionScroll_->setWidgetResizable(true);
	selectionBody_ = new QWidget(selectionScroll_);
	selectionLayout_ = new QVBoxLayout(selectionBody_);
	selectionLayout_->setContentsMargins(4, 4, 4, 4);
	selectionLayout_->setSpacing(2);
	selectionScroll_->setWidget(selectionBody_);
	outer->addWidget(selectionScroll_, 1);

	auto *btnRow = new QHBoxLayout();
	refreshBtn_ = new QPushButton(tr("Refresh list"), this);
	connect(refreshBtn_, &QPushButton::clicked, this, &StatusDock::repopulateSelection);
	btnRow->addWidget(refreshBtn_);
	clearBtn_ = new QPushButton(tr("Clear (auto)"), this);
	connect(clearBtn_, &QPushButton::clicked, this, &StatusDock::clearSelection);
	btnRow->addWidget(clearBtn_);
	btnRow->addStretch();
	outer->addLayout(btnRow);

	auto *stepRow = new QHBoxLayout();
	stepRow->addWidget(new QLabel(tr("Hotkey step:"), this));
	stepSpin_ = new QDoubleSpinBox(this);
	stepSpin_->setRange(0.5, 24.0);
	stepSpin_->setSingleStep(0.5);
	stepSpin_->setDecimals(1);
	stepSpin_->setSuffix(tr(" dB"));
	stepSpin_->setValue(cfg_.nudge_step_db());
	connect(stepSpin_, &QDoubleSpinBox::valueChanged, this, &StatusDock::onStepChanged);
	stepRow->addWidget(stepSpin_);
	stepRow->addStretch();
	outer->addLayout(stepRow);

	auto *sep2 = new QFrame(this);
	sep2->setFrameShape(QFrame::HLine);
	sep2->setFrameShadow(QFrame::Sunken);
	outer->addWidget(sep2);

	headerLabel_ = new QLabel(this);
	QFont bold = headerLabel_->font();
	bold.setBold(true);
	headerLabel_->setFont(bold);
	headerLabel_->setWordWrap(true);
	outer->addWidget(headerLabel_);

	bodyLabel_ = new QLabel(this);
	bodyLabel_->setWordWrap(true);
	bodyLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
	QFont mono("Menlo");
	mono.setStyleHint(QFont::Monospace);
	bodyLabel_->setFont(mono);
	outer->addWidget(bodyLabel_);

	repopulateSelection();
	updateEnabledState();

	timer_ = new QTimer(this);
	timer_->setInterval(250);
	connect(timer_, &QTimer::timeout, this, &StatusDock::refresh);
	timer_->start();

	refresh();
}

void StatusDock::updateEnabledState()
{
	bool dca = dcaCheck_->isChecked();
	selectionLabel_->setEnabled(!dca);
	selectionScroll_->setEnabled(!dca);
	refreshBtn_->setEnabled(!dca);
	clearBtn_->setEnabled(!dca);
}

void StatusDock::repopulateSelection()
{
	QLayoutItem *item;
	while ((item = selectionLayout_->takeAt(0)) != nullptr) {
		if (item->widget())
			item->widget()->deleteLater();
		delete item;
	}

	for (const auto &name : browser_source_names()) {
		auto *cb = new QCheckBox(QString::fromStdString(name), selectionBody_);
		cb->setChecked(cfg_.is_selected(name));
		// Connect AFTER setChecked so the initial state doesn't fire the slot.
		connect(cb, &QCheckBox::toggled, this, [this, name](bool on) {
			cfg_.set_selected(name, on);
			cfg_.save();
			tracker_.resolve_current();
		});
		selectionLayout_->addWidget(cb);
	}
	selectionLayout_->addStretch();
}

void StatusDock::clearSelection()
{
	cfg_.clear_selection();
	cfg_.save();
	repopulateSelection();
	tracker_.resolve_current();
}

void StatusDock::onDcaToggled(bool checked)
{
	cfg_.set_dca_mode(checked);
	cfg_.save();
	updateEnabledState();
	refresh();
}

void StatusDock::refresh()
{
	if (dcaCheck_->isChecked()) {
		auto entries = browser_dca::snapshot();
		headerLabel_->setText(tr("DCA mode - %1 browser source(s):").arg(entries.size()));
		if (entries.empty()) {
			bodyLabel_->setText(tr("(no browser sources found)"));
			return;
		}
		QString text;
		for (const auto &e : entries) {
			text += QString::fromStdString(e.name);
			text += QStringLiteral("\n    ");
			text += db_text(e.db);
			if (e.muted)
				text += tr("   (muted)");
			text += QStringLiteral("\n");
		}
		bodyLabel_->setText(text.trimmed());
		return;
	}

	std::string name = ctl_.name();
	if (name.empty()) {
		headerLabel_->setText(cfg_.has_selection()
					      ? tr("Controlling: - (no checked browser on the live scene)")
					      : tr("Controlling: - (no browser source on the live scene)"));
		bodyLabel_->setText(QString());
		return;
	}
	headerLabel_->setText(tr("Controlling: %1").arg(QString::fromStdString(name)));

	float db = 0.0f;
	bool muted = false;
	bool haveDb = ctl_.get_db(db);
	ctl_.get_mute(muted);

	QString level = haveDb ? tr("Level: %1").arg(db_text(db)) : tr("Level: -");
	if (muted)
		level += tr("   (muted)");
	bodyLabel_->setText(level);
}

void StatusDock::onStepChanged(double value)
{
	cfg_.set_nudge_step_db((float)value);
	cfg_.save();
}
