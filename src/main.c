#include "log.h"
#include "manager.h"

int main()
{
	LimeWM *wm = lime_window_manager_create();
	int ret = lime_window_manager_init(wm);
	if (ret != 0)
	{
		return -1;
	}
	lime_window_manager_run(wm);
	return 0;
}
