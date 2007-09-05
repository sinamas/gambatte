/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "inputdialog.h"

#include <QLineEdit>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>

#include "SDL_Joystick/include/SDL_joystick.h"

static QString keyToString(int key) {
	switch (key) {
	case Qt::Key_Escape: return QString("Escape");
	case Qt::Key_Tab: return QString("Tab");
	case Qt::Key_Backtab: return QString("Backtab");
	case Qt::Key_Backspace: return QString("Backspace");
	case Qt::Key_Return: return QString("Return");
	case Qt::Key_Enter: return QString("Enter");
	case Qt::Key_Insert: return QString("Insert");
	case Qt::Key_Delete: return QString("Delete");
	case Qt::Key_Pause: return QString("Pause");
	case Qt::Key_Print: return QString("Print");
	case Qt::Key_SysReq: return QString("SysReq");
	case Qt::Key_Clear: return QString("Clear");
	case Qt::Key_Home: return QString("Home");
	case Qt::Key_End: return QString("End");
	case Qt::Key_Left: return QString("Left");
	case Qt::Key_Up: return QString("Up");
	case Qt::Key_Right: return QString("Right");
	case Qt::Key_Down: return QString("Down");
	case Qt::Key_PageUp: return QString("PageUp");
	case Qt::Key_PageDown: return QString("PageDown");
	case Qt::Key_Shift: return QString("Shift");
	case Qt::Key_Control: return QString("Control");
	case Qt::Key_Meta: return QString("Meta");
	case Qt::Key_Alt: return QString("Alt");
	case Qt::Key_AltGr: return QString("AltGr");
	case Qt::Key_CapsLock: return QString("CapsLock");
	case Qt::Key_NumLock: return QString("NumLock");
	case Qt::Key_ScrollLock: return QString("ScrollLock");
	case Qt::Key_F1: return QString("F1");
	case Qt::Key_F2: return QString("F2");
	case Qt::Key_F3: return QString("F3");
	case Qt::Key_F4: return QString("F4");
	case Qt::Key_F5: return QString("F5");
	case Qt::Key_F6: return QString("F6");
	case Qt::Key_F7: return QString("F7");
	case Qt::Key_F8: return QString("F8");
	case Qt::Key_F9: return QString("F9");
	case Qt::Key_F10: return QString("F10");
	case Qt::Key_F11: return QString("F11");
	case Qt::Key_F12: return QString("F12");
	case Qt::Key_F13: return QString("F13");
	case Qt::Key_F14: return QString("F14");
	case Qt::Key_F15: return QString("F15");
	case Qt::Key_F16: return QString("F16");
	case Qt::Key_F17: return QString("F17");
	case Qt::Key_F18: return QString("F18");
	case Qt::Key_F19: return QString("F19");
	case Qt::Key_F20: return QString("F20");
	case Qt::Key_F21: return QString("F21");
	case Qt::Key_F22: return QString("F22");
	case Qt::Key_F23: return QString("F23");
	case Qt::Key_F24: return QString("F24");
	case Qt::Key_F25: return QString("F25");
	case Qt::Key_F26: return QString("F26");
	case Qt::Key_F27: return QString("F27");
	case Qt::Key_F28: return QString("F28");
	case Qt::Key_F29: return QString("F29");
	case Qt::Key_F30: return QString("F30");
	case Qt::Key_F31: return QString("F31");
	case Qt::Key_F32: return QString("F32");
	case Qt::Key_F33: return QString("F33");
	case Qt::Key_F34: return QString("F34");
	case Qt::Key_F35: return QString("F35");
	case Qt::Key_Super_L: return QString("Super_L");
	case Qt::Key_Super_R: return QString("Super_R");
	case Qt::Key_Menu: return QString("Menu");
	case Qt::Key_Hyper_L: return QString("Hyper_L");
	case Qt::Key_Hyper_R: return QString("Hyper_R");
	case Qt::Key_Help: return QString("Help");
	case Qt::Key_Direction_L: return QString("Direction_L");
	case Qt::Key_Direction_R: return QString("Direction_R");
	case Qt::Key_Space: return QString("Space");
	case Qt::Key_Exclam: return QString("Exclam");
	case Qt::Key_QuoteDbl: return QString("QuoteDbl");
	case Qt::Key_NumberSign: return QString("NumberSign");
	case Qt::Key_Dollar: return QString("Dollar");
	case Qt::Key_Percent: return QString("Percent");
	case Qt::Key_Ampersand: return QString("Ampersand");
	case Qt::Key_Apostrophe: return QString("Apostrophe");
	case Qt::Key_ParenLeft: return QString("ParenLeft");
	case Qt::Key_ParenRight: return QString("ParenRight");
	case Qt::Key_Asterisk: return QString("Asterisk");
	case Qt::Key_Plus: return QString("Plus");
	case Qt::Key_Comma: return QString("Comma");
	case Qt::Key_Minus: return QString("Minus");
	case Qt::Key_Period: return QString("Period");
	case Qt::Key_Slash: return QString("Slash");
	case Qt::Key_0: return QString("0");
	case Qt::Key_1: return QString("1");
	case Qt::Key_2: return QString("2");
	case Qt::Key_3: return QString("3");
	case Qt::Key_4: return QString("4");
	case Qt::Key_5: return QString("5");
	case Qt::Key_6: return QString("6");
	case Qt::Key_7: return QString("7");
	case Qt::Key_8: return QString("8");
	case Qt::Key_9: return QString("9");
	case Qt::Key_Colon: return QString("Colon");
	case Qt::Key_Semicolon: return QString("Semicolon");
	case Qt::Key_Less: return QString("Less");
	case Qt::Key_Equal: return QString("Equal");
	case Qt::Key_Greater: return QString("Greater");
	case Qt::Key_Question: return QString("Question");
	case Qt::Key_At: return QString("At");
	case Qt::Key_A: return QString("A");
	case Qt::Key_B: return QString("B");
	case Qt::Key_C: return QString("C");
	case Qt::Key_D: return QString("D");
	case Qt::Key_E: return QString("E");
	case Qt::Key_F: return QString("F");
	case Qt::Key_G: return QString("G");
	case Qt::Key_H: return QString("H");
	case Qt::Key_I: return QString("I");
	case Qt::Key_J: return QString("J");
	case Qt::Key_K: return QString("K");
	case Qt::Key_L: return QString("L");
	case Qt::Key_M: return QString("M");
	case Qt::Key_N: return QString("N");
	case Qt::Key_O: return QString("O");
	case Qt::Key_P: return QString("P");
	case Qt::Key_Q: return QString("Q");
	case Qt::Key_R: return QString("R");
	case Qt::Key_S: return QString("S");
	case Qt::Key_T: return QString("T");
	case Qt::Key_U: return QString("U");
	case Qt::Key_V: return QString("V");
	case Qt::Key_W: return QString("W");
	case Qt::Key_X: return QString("X");
	case Qt::Key_Y: return QString("Y");
	case Qt::Key_Z: return QString("Z");
	case Qt::Key_BracketLeft: return QString("BracketLeft");
	case Qt::Key_Backslash: return QString("Backslash");
	case Qt::Key_BracketRight: return QString("BracketRight");
	case Qt::Key_AsciiCircum: return QString("AsciiCircum");
	case Qt::Key_Underscore: return QString("Underscore");
	case Qt::Key_QuoteLeft: return QString("QuoteLeft");
	case Qt::Key_BraceLeft: return QString("BraceLeft");
	case Qt::Key_Bar: return QString("Bar");
	case Qt::Key_BraceRight: return QString("BraceRight");
	case Qt::Key_AsciiTilde: return QString("AsciiTilde");
	case Qt::Key_nobreakspace: return QString("nobreakspace");
	case Qt::Key_exclamdown: return QString("exclamdown");
	case Qt::Key_cent: return QString("cent");
	case Qt::Key_sterling: return QString("sterling");
	case Qt::Key_currency: return QString("currency");
	case Qt::Key_yen: return QString("yen");
	case Qt::Key_brokenbar: return QString("brokenbar");
	case Qt::Key_section: return QString("section");
	case Qt::Key_diaeresis: return QString("diaeresis");
	case Qt::Key_copyright: return QString("copyright");
	case Qt::Key_ordfeminine: return QString("ordfeminine");
	case Qt::Key_guillemotleft: return QString("guillemotleft");
	case Qt::Key_notsign: return QString("notsign");
	case Qt::Key_hyphen: return QString("hyphen");
	case Qt::Key_registered: return QString("registered");
	case Qt::Key_macron: return QString("macron");
	case Qt::Key_degree: return QString("degree");
	case Qt::Key_plusminus: return QString("plusminus");
	case Qt::Key_twosuperior: return QString("twosuperior");
	case Qt::Key_threesuperior: return QString("threesuperior");
	case Qt::Key_acute: return QString("acute");
	case Qt::Key_mu: return QString("mu");
	case Qt::Key_paragraph: return QString("paragraph");
	case Qt::Key_periodcentered: return QString("periodcentered");
	case Qt::Key_cedilla: return QString("cedilla");
	case Qt::Key_onesuperior: return QString("onesuperior");
	case Qt::Key_masculine: return QString("masculine");
	case Qt::Key_guillemotright: return QString("guillemotright");
	case Qt::Key_onequarter: return QString("onequarter");
	case Qt::Key_onehalf: return QString("onehalf");
	case Qt::Key_threequarters: return QString("threequarters");
	case Qt::Key_questiondown: return QString("questiondown");
	case Qt::Key_Agrave: return QString("Agrave");
	case Qt::Key_Aacute: return QString("Aacute");
	case Qt::Key_Acircumflex: return QString("Acircumflex");
	case Qt::Key_Atilde: return QString("Atilde");
	case Qt::Key_Adiaeresis: return QString("Adiaeresis");
	case Qt::Key_Aring: return QString("Aring");
	case Qt::Key_AE: return QString("AE");
	case Qt::Key_Ccedilla: return QString("Ccedilla");
	case Qt::Key_Egrave: return QString("Egrave");
	case Qt::Key_Eacute: return QString("Eacute");
	case Qt::Key_Ecircumflex: return QString("Ecircumflex");
	case Qt::Key_Ediaeresis: return QString("Ediaeresis");
	case Qt::Key_Igrave: return QString("Igrave");
	case Qt::Key_Iacute: return QString("Iacute");
	case Qt::Key_Icircumflex: return QString("Icircumflex");
	case Qt::Key_Idiaeresis: return QString("Idiaeresis");
	case Qt::Key_ETH: return QString("ETH");
	case Qt::Key_Ntilde: return QString("Ntilde");
	case Qt::Key_Ograve: return QString("Ograve");
	case Qt::Key_Oacute: return QString("Oacute");
	case Qt::Key_Ocircumflex: return QString("Ocircumflex");
	case Qt::Key_Otilde: return QString("Otilde");
	case Qt::Key_Odiaeresis: return QString("Odiaeresis");
	case Qt::Key_multiply: return QString("multiply");
	case Qt::Key_Ooblique: return QString("Ooblique");
	case Qt::Key_Ugrave: return QString("Ugrave");
	case Qt::Key_Uacute: return QString("Uacute");
	case Qt::Key_Ucircumflex: return QString("Ucircumflex");
	case Qt::Key_Udiaeresis: return QString("Udiaeresis");
	case Qt::Key_Yacute: return QString("Yacute");
	case Qt::Key_THORN: return QString("THORN");
	case Qt::Key_ssharp: return QString("ssharp");
	case Qt::Key_division: return QString("division");
	case Qt::Key_ydiaeresis: return QString("ydiaeresis");
	case Qt::Key_Multi_key: return QString("Multi_key");
	case Qt::Key_Codeinput: return QString("Codeinput");
	case Qt::Key_SingleCandidate: return QString("SingleCandidate");
	case Qt::Key_MultipleCandidate: return QString("MultipleCandidate");
	case Qt::Key_PreviousCandidate: return QString("PreviousCandidate");
	case Qt::Key_Mode_switch: return QString("Mode_switch");
	case Qt::Key_Kanji: return QString("Kanji");
	case Qt::Key_Muhenkan: return QString("Muhenkan");
	case Qt::Key_Henkan: return QString("Henkan");
	case Qt::Key_Romaji: return QString("Romaji");
	case Qt::Key_Hiragana: return QString("Hiragana");
	case Qt::Key_Katakana: return QString("Katakana");
	case Qt::Key_Hiragana_Katakana: return QString("Hiragana_Katakana");
	case Qt::Key_Zenkaku: return QString("Zenkaku");
	case Qt::Key_Hankaku: return QString("Hankaku");
	case Qt::Key_Zenkaku_Hankaku: return QString("Zenkaku_Hankaku");
	case Qt::Key_Touroku: return QString("Touroku");
	case Qt::Key_Massyo: return QString("Massyo");
	case Qt::Key_Kana_Lock: return QString("Kana_Lock");
	case Qt::Key_Kana_Shift: return QString("Kana_Shift");
	case Qt::Key_Eisu_Shift: return QString("Eisu_Shift");
	case Qt::Key_Eisu_toggle: return QString("Eisu_toggle");
	case Qt::Key_Hangul: return QString("Hangul");
	case Qt::Key_Hangul_Start: return QString("Hangul_Start");
	case Qt::Key_Hangul_End: return QString("Hangul_End");
	case Qt::Key_Hangul_Hanja: return QString("Hangul_Hanja");
	case Qt::Key_Hangul_Jamo: return QString("Hangul_Jamo");
	case Qt::Key_Hangul_Romaja: return QString("Hangul_Romaja");
	case Qt::Key_Hangul_Jeonja: return QString("Hangul_Jeonja");
	case Qt::Key_Hangul_Banja: return QString("Hangul_Banja");
	case Qt::Key_Hangul_PreHanja: return QString("Hangul_PreHanja");
	case Qt::Key_Hangul_PostHanja: return QString("Hangul_PostHanja");
	case Qt::Key_Hangul_Special: return QString("Hangul_Special");
	case Qt::Key_Dead_Grave: return QString("Dead_Grave");
	case Qt::Key_Dead_Acute: return QString("Dead_Acute");
	case Qt::Key_Dead_Circumflex: return QString("Dead_Circumflex");
	case Qt::Key_Dead_Tilde: return QString("Dead_Tilde");
	case Qt::Key_Dead_Macron: return QString("Dead_Macron");
	case Qt::Key_Dead_Breve: return QString("Dead_Breve");
	case Qt::Key_Dead_Abovedot: return QString("Dead_Abovedot");
	case Qt::Key_Dead_Diaeresis: return QString("Dead_Diaeresis");
	case Qt::Key_Dead_Abovering: return QString("Dead_Abovering");
	case Qt::Key_Dead_Doubleacute: return QString("Dead_Doubleacute");
	case Qt::Key_Dead_Caron: return QString("Dead_Caron");
	case Qt::Key_Dead_Cedilla: return QString("Dead_Cedilla");
	case Qt::Key_Dead_Ogonek: return QString("Dead_Ogonek");
	case Qt::Key_Dead_Iota: return QString("Dead_Iota");
	case Qt::Key_Dead_Voiced_Sound: return QString("Dead_Voiced_Sound");
	case Qt::Key_Dead_Semivoiced_Sound: return QString("Dead_Semivoiced_Sound");
	case Qt::Key_Dead_Belowdot: return QString("Dead_Belowdot");
	case Qt::Key_Dead_Hook: return QString("Dead_Hook");
	case Qt::Key_Dead_Horn: return QString("Dead_Horn");
	case Qt::Key_Back: return QString("Back");
	case Qt::Key_Forward: return QString("Forward");
	case Qt::Key_Stop: return QString("Stop");
	case Qt::Key_Refresh: return QString("Refresh");
	case Qt::Key_VolumeDown: return QString("VolumeDown");
	case Qt::Key_VolumeMute: return QString("VolumeMute");
	case Qt::Key_VolumeUp: return QString("VolumeUp");
	case Qt::Key_BassBoost: return QString("BassBoost");
	case Qt::Key_BassUp: return QString("BassUp");
	case Qt::Key_BassDown: return QString("BassDown");
	case Qt::Key_TrebleUp: return QString("TrebleUp");
	case Qt::Key_TrebleDown: return QString("TrebleDown");
	case Qt::Key_MediaPlay: return QString("MediaPlay");
	case Qt::Key_MediaStop: return QString("MediaStop");
	case Qt::Key_MediaPrevious: return QString("MediaPrevious");
	case Qt::Key_MediaNext: return QString("MediaNext");
	case Qt::Key_MediaRecord: return QString("MediaRecord");
	case Qt::Key_HomePage: return QString("HomePage");
	case Qt::Key_Favorites: return QString("Favorites");
	case Qt::Key_Search: return QString("Search");
	case Qt::Key_Standby: return QString("Standby");
	case Qt::Key_OpenUrl: return QString("OpenUrl");
	case Qt::Key_LaunchMail: return QString("LaunchMail");
	case Qt::Key_LaunchMedia: return QString("LaunchMedia");
	case Qt::Key_Launch0: return QString("Launch0");
	case Qt::Key_Launch1: return QString("Launch1");
	case Qt::Key_Launch2: return QString("Launch2");
	case Qt::Key_Launch3: return QString("Launch3");
	case Qt::Key_Launch4: return QString("Launch4");
	case Qt::Key_Launch5: return QString("Launch5");
	case Qt::Key_Launch6: return QString("Launch6");
	case Qt::Key_Launch7: return QString("Launch7");
	case Qt::Key_Launch8: return QString("Launch8");
	case Qt::Key_Launch9: return QString("Launch9");
	case Qt::Key_LaunchA: return QString("LaunchA");
	case Qt::Key_LaunchB: return QString("LaunchB");
	case Qt::Key_LaunchC: return QString("LaunchC");
	case Qt::Key_LaunchD: return QString("LaunchD");
	case Qt::Key_LaunchE: return QString("LaunchE");
	case Qt::Key_LaunchF: return QString("LaunchF");
	case Qt::Key_MediaLast: return QString("MediaLast");
	case Qt::Key_unknown: return QString("unknown");
	case Qt::Key_Call: return QString("Call");
	case Qt::Key_Context1: return QString("Context1");
	case Qt::Key_Context2: return QString("Context2");
	case Qt::Key_Context3: return QString("Context3");
	case Qt::Key_Context4: return QString("Context4");
	case Qt::Key_Flip: return QString("Flip");
	case Qt::Key_Hangup: return QString("Hangup");
	case Qt::Key_No: return QString("No");
	case Qt::Key_Select: return QString("Select");
	case Qt::Key_Yes: return QString("Yes");
	case Qt::Key_Execute: return QString("Execute");
	case Qt::Key_Printer: return QString("Printer");
	case Qt::Key_Play: return QString("Play");
	case Qt::Key_Sleep: return QString("Sleep");
	case Qt::Key_Zoom: return QString("Zoom");
	case Qt::Key_Cancel: return QString("Cancel");
	default: return QString("");
	}
}

class InputBox : public QLineEdit {
	SDL_Event data;
	int timerId;
	
protected:
	void focusInEvent(QFocusEvent *event);
	void focusOutEvent(QFocusEvent *event);
	void keyPressEvent(QKeyEvent *e) { setData(e->key()); }
	void timerEvent(QTimerEvent */*event*/);
	
public:
	InputBox();
	void setData(const SDL_Event &data) { setData(data.id, data.value); }
	void setData(unsigned id,  unsigned value = KBD_VALUE);
	const SDL_Event& getData() const { return data; }
};

InputBox::InputBox() : timerId(0) {
	setReadOnly(true);
	setData(0);
}

void InputBox::focusInEvent(QFocusEvent *event) {
	if (!timerId)
		timerId = startTimer(20);
	
	QLineEdit::focusInEvent(event);
}

void InputBox::focusOutEvent(QFocusEvent *event) {
	killTimer(timerId);
	timerId = 0;
	
	QLineEdit::focusOutEvent(event);
}

void InputBox::timerEvent(QTimerEvent */*event*/) {
	SDL_ClearEvents();
	SDL_JoystickUpdate();
	
	SDL_Event ev;
	int value = 0;
	unsigned id = 0;
	
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_JOYAXISMOTION:
			if (ev.value > JSPAXIS_VALUE) {
				value = JSPAXIS_VALUE;
			} else if (ev.value < JSNAXIS_VALUE) {
				value = JSNAXIS_VALUE;
			} else
				continue;
			break;
		case SDL_JOYBUTTONCHANGE:
			value = JSBUTTON_VALUE;
			break;
		default: continue;
		}
		
		id = ev.id;
	}
	
	if (id)
		setData(id, value);
}

void InputBox::setData(const unsigned id, const unsigned value) {
	data.id = id;
	data.value = value;
	
	if (value) {
		QString str(SDL_JoystickName(data.dev_num));
		str.append(' ');
		
		switch (data.type) {
		case SDL_JOYAXISMOTION:
			str.append("Axis ");
			str.append(QString::number(data.num));
			str.append(' ');
			str.append(data.value < 0 ? '-' : '+');
			break;
		case SDL_JOYBUTTONCHANGE:
			str.append("Button ");
			str.append(QString::number(data.num));
			break;
		}
		
		setText(str);
	} else
		setText(keyToString(id));
}

InputDialog::InputDialog(QWidget *parent)
 : QDialog(parent)
{
	setWindowTitle(tr("Input settings"));
	
	upBox = new InputBox;
	downBox = new InputBox;
	leftBox = new InputBox;
	rightBox = new InputBox;
	aBox = new InputBox;
	bBox = new InputBox;
	startBox = new InputBox;
	selectBox = new InputBox;
	
	QVBoxLayout *mainLayout = new QVBoxLayout;
	QHBoxLayout *topLayout = new QHBoxLayout;
	QVBoxLayout *vLayout;
	QHBoxLayout *hLayout;
	
	hLayout = new QHBoxLayout;
	
	vLayout = new QVBoxLayout;
	vLayout->addWidget(new QLabel(tr("Up:")));
	vLayout->addWidget(new QLabel(tr("Down:")));
	vLayout->addWidget(new QLabel(tr("Left:")));
	vLayout->addWidget(new QLabel(tr("Right:")));
	hLayout->addLayout(vLayout);
	
	vLayout = new QVBoxLayout;
	vLayout->addWidget(upBox);
	vLayout->addWidget(downBox);
	vLayout->addWidget(leftBox);
	vLayout->addWidget(rightBox);
	hLayout->addLayout(vLayout);
	
	topLayout->addLayout(hLayout);
	hLayout = new QHBoxLayout;
	
	vLayout = new QVBoxLayout;
	vLayout->addWidget(new QLabel(tr("A:")));
	vLayout->addWidget(new QLabel(tr("B:")));
	vLayout->addWidget(new QLabel(tr("Start:")));
	vLayout->addWidget(new QLabel(tr("Select:")));
	hLayout->addLayout(vLayout);
	
	vLayout = new QVBoxLayout;
	vLayout->addWidget(aBox);
	vLayout->addWidget(bBox);
	vLayout->addWidget(startBox);
	vLayout->addWidget(selectBox);
	hLayout->addLayout(vLayout);
	
	topLayout->addLayout(hLayout);
	mainLayout->addLayout(topLayout);
	
	QPushButton *okButton = new QPushButton(tr("OK"));
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	hLayout = new QHBoxLayout;
	hLayout->addWidget(okButton);
	hLayout->addWidget(cancelButton);
	mainLayout->addLayout(hLayout);
	mainLayout->setAlignment(hLayout, Qt::AlignBottom | Qt::AlignRight);
	okButton->setDefault(true);
	
	setLayout(mainLayout);
	
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	
	QSettings settings;
	settings.beginGroup("input");
	upData.id = settings.value("upKey", Qt::Key_Up).toInt();
	upData.value = settings.value("upValue", KBD_VALUE).toInt();
	downData.id = settings.value("downKey", Qt::Key_Down).toInt();
	downData.value = settings.value("downValue", KBD_VALUE).toInt();
	leftData.id = settings.value("leftKey", Qt::Key_Left).toInt();
	leftData.value = settings.value("leftValue", KBD_VALUE).toInt();
	rightData.id = settings.value("rightKey", Qt::Key_Right).toInt();
	rightData.value = settings.value("rightValue", KBD_VALUE).toInt();
	aData.id = settings.value("aKey", Qt::Key_D).toInt();
	aData.value = settings.value("aValue", KBD_VALUE).toInt();
	bData.id = settings.value("bKey", Qt::Key_C).toInt();
	bData.value = settings.value("bValue", KBD_VALUE).toInt();
	startData.id = settings.value("startKey", Qt::Key_Return).toInt();
	startData.value = settings.value("startValue", KBD_VALUE).toInt();
	selectData.id = settings.value("selectKey", Qt::Key_Shift).toInt();
	selectData.value = settings.value("selectValue", KBD_VALUE).toInt();
	settings.endGroup();
	
	restore();
}

InputDialog::~InputDialog() {
	QSettings settings;
	settings.beginGroup("input");
	settings.setValue("upKey", upData.id);
	settings.setValue("upValue", upData.value);
	settings.setValue("downKey", downData.id);
	settings.setValue("downValue", downData.value);
	settings.setValue("leftKey", leftData.id);
	settings.setValue("leftValue", leftData.value);
	settings.setValue("rightKey", rightData.id);
	settings.setValue("rightValue", rightData.value);
	settings.setValue("aKey", aData.id);
	settings.setValue("aValue", aData.value);
	settings.setValue("bKey", bData.id);
	settings.setValue("bValue", bData.value);
	settings.setValue("startKey", startData.id);
	settings.setValue("startValue", startData.value);
	settings.setValue("selectKey", selectData.id);
	settings.setValue("selectValue", selectData.value);
	settings.endGroup();
}

void InputDialog::store() {
	upData = upBox->getData();
	downData = downBox->getData();
	leftData = leftBox->getData();
	rightData = rightBox->getData();
	aData = aBox->getData();
	bData = bBox->getData();
	startData = startBox->getData();
	selectData = selectBox->getData();
}

void InputDialog::restore() {
	upBox->setData(upData);
	downBox->setData(downData);
	leftBox->setData(leftData);
	rightBox->setData(rightData);
	aBox->setData(aData);
	bBox->setData(bData);
	startBox->setData(startData);
	selectBox->setData(selectData);
}

void InputDialog::accept() {
	store();
	QDialog::accept();
}

void InputDialog::reject() {
	restore();
	QDialog::reject();
}
