/* Copyright (C) 2011-2021 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SynthStateMonitor.h"

#include "SynthRoute.h"
#include "ui_SynthWidget.h"
#include "font_6x8.h"

static const MasterClockNanos MINIMUM_UPDATE_INTERVAL_NANOS = 30 * MasterClock::NANOS_PER_MILLISECOND;

static const QColor COLOR_GRAY = QColor(100, 100, 100);
static const QColor COLOR_GREEN = Qt::green;
static const QColor lcdBgColor(98, 127, 0);
static const QColor lcdFgColor(232, 254, 0);
static const QColor partialStateColor[] = {COLOR_GRAY, Qt::red, Qt::yellow, Qt::green};

using namespace MT32Emu;

SynthStateMonitor::SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute) :
	synthRoute(useSynthRoute),
	ui(ui),
	lcdWidget(*this, ui->synthFrame),
	midiMessageLED(&COLOR_GRAY, ui->midiMessageFrame)
{
	partialCount = useSynthRoute->getPartialCount();
	allocatePartialsData();

	lcdWidget.setMinimumSize(254, 40);
	ui->synthFrameLayout->insertWidget(1, &lcdWidget);
	midiMessageLED.setMinimumSize(10, 2);
	ui->midiMessageLayout->addWidget(&midiMessageLED, 0, Qt::AlignHCenter);

	for (int i = 0; i < 9; i++) {
		patchNameLabel[i] = new QLabel(ui->polyStateGrid->widget());
		ui->polyStateGrid->addWidget(patchNameLabel[i], i, 0);

		partStateWidget[i] = new PartStateWidget(i, *this, ui->polyStateGrid->widget());
		partStateWidget[i]->setMinimumSize(480, 16);
		partStateWidget[i]->setMaximumSize(480, 16);
		ui->polyStateGrid->addWidget(partStateWidget[i], i, 1);
	}

	handleSynthStateChange(synthRoute->getState() == SynthRouteState_OPEN ? SynthState_OPEN : SynthState_CLOSED);
	synthRoute->connectSynth(SIGNAL(stateChanged(SynthState)), this, SLOT(handleSynthStateChange(SynthState)));
	synthRoute->connectSynth(SIGNAL(audioBlockRendered()), this, SLOT(handleUpdate()));
	synthRoute->connectReportHandler(SIGNAL(programChanged(int, QString, QString)), this, SLOT(handleProgramChanged(int, QString, QString)));
	synthRoute->connectReportHandler(SIGNAL(polyStateChanged(int)), this, SLOT(handlePolyStateChanged(int)));
}

SynthStateMonitor::~SynthStateMonitor() {
	for (int i = 0; i < 9; i++) {
		delete partStateWidget[i];
		delete patchNameLabel[i];
	}
	freePartialsData();
}

void SynthStateMonitor::enableMonitor(bool enable) {
	if (enable) {
		enabled = true;
		previousUpdateNanos = MasterClock::getClockNanos() - MINIMUM_UPDATE_INTERVAL_NANOS;
	} else {
		enabled = false;
	}
}

void SynthStateMonitor::handleSynthStateChange(SynthState state) {
	enableMonitor(state == SynthState_OPEN);
	midiMessageLED.setColor(&COLOR_GRAY);

	uint newPartialCount = synthRoute->getPartialCount();
	if (partialCount == newPartialCount || state != SynthState_OPEN) {
		for (unsigned int i = 0; i < partialCount; i++) {
			partialStateLED[i]->setColor(&partialStateColor[PartialState_INACTIVE]);
		}
	} else {
		freePartialsData();
		partialCount = newPartialCount;
		allocatePartialsData();
	}

	for (int i = 0; i < 9; i++) {
		patchNameLabel[i]->setText((i < 8) ? synthRoute->getPatchName(i) : "Rhythm Channel");
		partStateWidget[i]->update();
	}
}

void SynthStateMonitor::handlePolyStateChanged(int partNum) {
	partStateWidget[partNum]->update();
}

void SynthStateMonitor::handleProgramChanged(int partNum, QString, QString patchName) {
	patchNameLabel[partNum]->setText(patchName);
}

void SynthStateMonitor::handleUpdate() {
	if (!enabled) return;
	MasterClockNanos nanosNow = MasterClock::getClockNanos();
	if (nanosNow - previousUpdateNanos < MINIMUM_UPDATE_INTERVAL_NANOS) return;
	previousUpdateNanos = nanosNow;
	bool midiMessageOn = synthRoute->getDisplayState(lcdWidget.lcdText);
	lcdWidget.update();
	midiMessageLED.setColor(midiMessageOn ? &COLOR_GREEN : &COLOR_GRAY);
}

void SynthStateMonitor::allocatePartialsData() {
	partialStates = new PartialState[partialCount];
	keysOfPlayingNotes = new Bit8u[partialCount];
	velocitiesOfPlayingNotes = new Bit8u[partialCount];

	partialStateLED = new LEDWidget*[partialCount];
	unsigned int partialColumnWidth;
	if (partialCount < 64) {
		partialColumnWidth = 4;
	} else if (partialCount < 128) {
		partialColumnWidth = 8;
	} else {
		partialColumnWidth = 16;
	}
	for (unsigned int i = 0; i < partialCount; i++) {
		partialStateLED[i] = new LEDWidget(&COLOR_GRAY, ui->partialStateGrid->widget());
		partialStateLED[i]->setMinimumSize(16, 16);
		partialStateLED[i]->setMaximumSize(16, 16);
		ui->partialStateGrid->addWidget(partialStateLED[i], i / partialColumnWidth, i % partialColumnWidth);
	}
}

void SynthStateMonitor::freePartialsData() {
	if (partialStateLED != NULL) {
		for (unsigned int i = 0; i < partialCount; i++) delete partialStateLED[i];
	}
	delete[] partialStateLED;
	partialStateLED = NULL;
	delete[] velocitiesOfPlayingNotes;
	velocitiesOfPlayingNotes = NULL;
	delete[] keysOfPlayingNotes;
	keysOfPlayingNotes = NULL;
	delete[] partialStates;
	partialStates = NULL;
}

LEDWidget::LEDWidget(const QColor *color, QWidget *parent) : QWidget(parent), colorProperty(color) {}

const QColor *LEDWidget::color() const {
	return colorProperty;
}

void LEDWidget::setColor(const QColor *newColor) {
	if (colorProperty != newColor) {
		colorProperty = newColor;
		update();
	}
}

void LEDWidget::paintEvent(QPaintEvent *paintEvent) {
	QPainter painter(this);
	if (colorProperty != NULL) {
		painter.fillRect(paintEvent->rect(), *colorProperty);
	}
}

PartStateWidget::PartStateWidget(int partNum, const SynthStateMonitor &monitor, QWidget *parent) : QWidget(parent), partNum(partNum), monitor(monitor) {}

void PartStateWidget::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	painter.fillRect(rect(), COLOR_GRAY);
	if (monitor.synthRoute->getState() != SynthRouteState_OPEN) return;
	uint playingNotes = monitor.synthRoute->getPlayingNotes(partNum, monitor.keysOfPlayingNotes, monitor.velocitiesOfPlayingNotes);
	while (playingNotes-- > 0) {
		uint velocity = monitor.velocitiesOfPlayingNotes[playingNotes];
		if (velocity == 0) continue;
		QColor color(2 * velocity, 255 - 2 * velocity, 0);
		uint x  = 5 * (monitor.keysOfPlayingNotes[playingNotes] - 12);
		painter.fillRect(x, 0, 5, 16, color);
	}
}

LCDWidget::LCDWidget(const SynthStateMonitor &monitor, QWidget *parent) :
	QWidget(parent),
	monitor(monitor),
	lcdOffBackground(":/images/LCDOff.gif"),
	lcdOnBackground(":/images/LCDOn.gif")
{}

void LCDWidget::paintEvent(QPaintEvent *) {
	QPainter lcdPainter(this);
	if (monitor.synthRoute->getState() != SynthRouteState_OPEN) {
		lcdPainter.drawPixmap(0, 0, lcdOffBackground);
		return;
	}
	lcdPainter.drawPixmap(0, 0, lcdOnBackground);
	lcdPainter.translate(7, 9);

	int xat, xstart, yat;
	xstart = 0;
	yat = 0;

	for (int i = 0; i < 20; i++) {
		uchar c = lcdText[i];

		// Map special characters to what we have defined in the font.
		if (c > 0x7f) c = 0x20;
		else if (c == 0x01) c = 0x80;
		else if (c == 0x02) c = 0x7c;
		else if (c < 0x20) c = 0x20;

		c -= 0x20;

		yat = 1;
		for (int t = 0; t < 8; t++) {
			xat = xstart;
			unsigned char fval = Font_6x8[c][t];
			for (int m = 4; m >= 0; --m) {
				if ((fval >> m) & 1) {
					lcdPainter.fillRect(xat, yat, 2, 2, lcdFgColor);
				} else {
					lcdPainter.fillRect(xat, yat, 2, 2, lcdBgColor);
				}
				xat += 2;
			}
			yat += 2;
			if (t == 6) yat += 2;
		}
		xstart += 12;
	}
}
