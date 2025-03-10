#pragma once
#include "Modal_gump.h"
#include "ConfigSetting_widget.h"
#include "Scrollable_widget.h"

class AdvancedOptions_gump : public Modal_gump{


	std::vector<ConfigSetting_widget::Definition>* settings;
	std::string                                    title;
	std::string                                    helpurl;
	std::shared_ptr<Font>                          font;
	Scrollable_widget *				            scroll;
	Gump_button*                                   apply;
	Gump_button*                                   cancel;
	Gump_button*                                   help;

public:
	AdvancedOptions_gump(
			std::vector<ConfigSetting_widget::Definition>* settings, std::string&&title,std::string&&helpurl);
			~AdvancedOptions_gump() override;

	bool mouse_down(int mx, int my, MouseButton button) override;
			bool mouse_up(int mx, int my, MouseButton button) override;
			bool mouse_drag(int mx, int my) override;

	bool mousewheel_down(int mx, int my) override;

	bool mousewheel_up(int mx, int my) override;

	void paint() override;

	bool run() override;
	
	// alternative 
	void paint_elems() override;

	void on_apply();
	void on_cancel();
	void on_help();

};
