#ifndef DWMCONTROL_H_
#define DWMCONTROL_H_

#include <vector>
#include <QWidget>

class BlitterWidget;

class DwmControl {
	const std::vector<BlitterWidget*> blitters_;
	int refreshCnt_;
	bool tripleBuffer_;

public:
	explicit DwmControl(const std::vector<BlitterWidget*> &blitters);
	void setDwmTripleBuffer(bool enable);
	void hideEvent();
	void showEvent();
	/** @return compositionChange */
	bool winEvent(const MSG &msg);
	void tick();
	void hwndChange(BlitterWidget *blitter);

	static bool hasDwmCapability();
	static bool isCompositingEnabled();
};

class DwmControlHwndChange {
	DwmControl &dwmc_;
public:
	explicit DwmControlHwndChange(DwmControl &dwmc) : dwmc_(dwmc) {}
	void operator()(BlitterWidget *blitter) const { dwmc_.hwndChange(blitter); }
};

#endif /* DWMCONTROL_H_ */
