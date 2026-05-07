#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h" 
#include <stddef.h>

#define DISP_ADD 0x3E
#define DISP_SDA 7 
#define DISP_SCL 8
#define DISP_i2c 100000


class DFR_display {
	public: 
		DFR_display(uint8_t rgbADD, uint8_t col, uint8_t row); 
		void init(); 
		void setCursor(uint8_t col, uint8_t row);
		void printstr(const char* str); 
		void dispRGB(uint8_t r, uint8_t g, uint8_t b); 
		i2c_master_bus_handle_t getBus() { return _bus; }

	private: 
		uint8_t _dispADD; 
		uint8_t _row; 
		uint8_t _col;

		i2c_master_dev_handle_t _rgbDisp; 
		i2c_master_dev_handle_t _lcdDisp;
		i2c_master_bus_handle_t _bus; 

		void dataSend(uint8_t data);
		void comSend(uint8_t com);
		void i2cWrite(i2c_master_dev_handle_t dev, const uint8_t* d, size_t leng);
		void setReg(uint8_t reg, uint8_t val); 
};
