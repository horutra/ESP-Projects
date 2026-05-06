#include "DFR_display.h" 

extern "C" void app_main(void){ 
	DFR_display lcd(0x2D, 16, 2); 
	lcd.init();
	lcd.dispRGB(191,0,240);

	lcd.setCursor(0,0);
	lcd.printstr("Hello CSE121!"); 

	lcd.setCursor(0,1);
	lcd.printstr("Arturo"); 

	while(true){
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
