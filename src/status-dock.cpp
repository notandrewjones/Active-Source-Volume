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
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

static const char *kAutoLabel = "(Auto: top-most browser in live scene)";

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

	auto *intro = new QLabel(tr("By default the hotkeys shift ONE browser source (so alert / donation / chat "
				    "browsers are left alone). Enable DCA mode to shift every browser source at once."),
				 this);
	intro->setWordWrap(true);
	outer->addWidget(intro);

	dcaCheck_ = new QCheckBox(tr("DCA mode: control all browser sources at once"), this);
	dcaCheck_->setChecked(cfg_.dca_mode());
	connect(dcaCheck_, &QCheckBox::toggled, this, &StatusDock::onDcaToggled);
	outer->addWidget(dcaCheck_);

	auto *ovRow = new QHBoxLayout();
	overrideLabel_ = new QLabel(tr("Controlled source:"), this);
	ovRow->addWidget(overrideLabel_);
	overrideCombo_ = new QComboBox(this);
	connect(overrideCombo_, &QComboBox::currentTextChanged, this, &StatusDock::onOverrideChanged);
	ovRow->addWidget(overrideCombo_, 1);
	outer->addLayout(ovRow);

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

	auto *refreshBtn = new QPushButton(tr("Refresh source list"), this);
	connect(refreshBtn, &QPushButton::clicked, this, &StatusDock::repopulateOverride);
	outer->addWidget(refreshBtn);

	auto *sep = new QFrame(this);
	sep->setFrameShape(QFrame::HLine);
	sep->setFrameShadow(QFrame::Sunken);
	outer->addWidget(sep);

	headerLabel_ = new QLabel(this);
	QFont bold = headerLabel_->font();
	bold.setBold(true);
	headerLabel_->setFont(bold);
	headerLabel_->setWordWrap(true);
	outer->addWidget(headerLabel_);

	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	bodyLabel_ = new QLabel(scroll);
	bodyLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
	bodyLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	QFont mono("Menlo");
	mono.setStyleHint(QFont::Monospace);
	bodyLabel_->setFont(mono);
	scroll->setWidget(bodyLabel_);
	outer->addWidget(scroll, 1);

	repopulateOverride();
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
	overrideCombo_->setEnabled(!dca);
	overrideLabel_->setEnabled(!dca);
}

void StatusDock::repopulateOverride()
{
	populating_ = true;
	overrideCombo_->clear();
	overrideCombo_->addItem(QString::fromUtf8(kAutoLabel));
	for (const auto &n : browser_source_names())
		overrideCombo_->addItem(QString::fromStdString(n));

	std::string ov = cfg_.override_source();
	if (!ov.empty()) {
		int idx = overrideCombo_->findText(QString::fromStdString(ov));
		if (idx >= 0) {
			overrideCombo_->setCurrentIndex(idx);
		} else {
			overrideCombo_->addItem(QString::fromStdString(ov) + tr(" (missing)"));
			overrideCombo_->setCurrentIndex(overrideCombo_->count() - 1);
		}
	} else {
		overrideCombo_->setCurrentIndex(0);
	}
	populating_ = false;
}

void StatusDock::onOverrideChanged(const QString &text)
{
	if (populating_)
		return;
	if (text == QString::fromUtf8(kAutoLabel)) {
		cfg_.set_override_source("");
	} else {
		QString clean = text;
		clean.remove(tr(" (missing)"));
		cfg_.set_override_source(clean.toStdString());
	}
	cfg_.save();
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
		headerLabel_->setText(tr("DCA mode — %1 browser source(s):").arg(entries.size()));
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
		headerLabel_->setText(tr("Controlling: — (no browser source selected)"));
		bodyLabel_->setText(QString());
		return;
	}
	headerLabel_->setText(tr("Controlling: %1").arg(QString::fromStdString(name)));

	float db = 0.0f;
	bool muted = false;
	bool haveDb = ctl_.get_db(db);
	ctl_.get_mute(muted);

	QString level = haveDb ? tr("Level: %1").arg(db_text(db)) : tr("Level: —");
	if (muted)
		level += tr("   (muted)");
	bodyLabel_->setText(level);
}

void StatusDock::onStepChanged(double value)
{
	cfg_.set_nudge_step_db((float)value);
	cfg_.save();
}
