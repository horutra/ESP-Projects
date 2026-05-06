#include "DFR_display.h" 

DFR_display::DFR_display(uint8_t rgbADD, uint8_t col, uint8_t row) 
	: _dispADD(rgbADD),  _row(row), _col(col),
	  _rgbDisp(nullptr), _lcdDisp(nullptr), _bus(nullptr){}

void DFR_display::i2cWrite(i2c_master_dev_handle_t dev, const uint8_t* d, size_t leng){
	i2c_master_transmit(dev, d, leng, 100); 
}

void DFR_display::comSend(uint8_t com) { 
	uint8_t buf[2] = {0x00, com};
	i2cWrite(_lcdDisp, buf, 2);
}

void DFR_display::dataSend(uint8_t data){
	uint8_t buf[2] = {0x40, data}; 
	i2cWrite(_lcdDisp, buf, 2);
}

void DFR_display::setReg(uint8_t regAddr, uint8_t value){
	uint8_t buf[2] = {regAddr, value};
	i2cWrite(_rgbDisp, buf, 2);
}

void DFR_display::init(){
	i2c_master_bus_config_t busConf = {};
	busConf.i2c_port = I2C_NUM_0;
	busConf.sda_io_num = (gpio_num_t)DISP_SDA;
	busConf.scl_io_num = (gpio_num_t)DISP_SCL;
	busConf.clk_source = I2C_CLK_SRC_DEFAULT;
	busConf.glitch_ignore_cnt = 7;
	busConf.flags.enable_internal_pullup = true;
	i2c_new_master_bus(&busConf, &_bus);

	i2c_device_config_t lcdConf = {};
	lcdConf.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	lcdConf.device_address = DISP_ADD; 
	lcdConf.scl_speed_hz = DISP_i2c;
	i2c_master_bus_add_device(_bus, &lcdConf, &_lcdDisp);

	i2c_device_config_t rgbConf = {};
	rgbConf.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	rgbConf.device_address = _dispADD;
	rgbConf.scl_speed_hz = DISP_i2c;
	i2c_master_bus_add_device(_bus, &rgbConf, &_rgbDisp); 

	vTaskDelay(pdMS_TO_TICKS(50));
	comSend(0x38); vTaskDelay(pdMS_TO_TICKS(5));
	comSend(0x38); vTaskDelay(pdMS_TO_TICKS(5));
	comSend(0x38); vTaskDelay(pdMS_TO_TICKS(1));
	comSend(0x38);
	comSend(0x0C);
	comSend(0x01); 
	vTaskDelay(pdMS_TO_TICKS(2));
	comSend(0x06); 

	setReg(0x00, 0x00); 
	setReg(0x01, 0x00);
       	setReg(0x08, 0xAA); 
}


void DFR_display::printstr(const char* str){
	while(*str) {
		dataSend((uint8_t)*str++);
	}
}

void DFR_display::dispRGB(uint8_t r, uint8_t g, uint8_t b){
	setReg(0x04, r);
	setReg(0x03, g);
	setReg(0x02, b);
}

void DFR_display::setCursor(uint8_t col, uint8_t row){ 
	uint8_t rowOffset = (row == 0) ? 0x00 : 0x40;
	comSend(0x80 | (col + rowOffset));
}

