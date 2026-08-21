
#include "handle_status.h"
#include "main.h"

handle_status::handle_status(/* args */) {}
handle_status::~handle_status() {}

TimedBlink PowerG(LED1_GRN);
// TimedBlink PowerR(LED1_RED);
TimedBlink InternetG(LED3_GRN);
TimedBlink InternetR(LED3_RED);
TimedBlink DecawaceG(LED2_GRN);
TimedBlink DecawaceR(LED2_RED);

status_ledINTERNET internet_ready_f;
status_ledDW decawave_ready_f;
status_ledPower power_ready_f;

void handle_status::begin(void)
{
	pinMode(LED1_GRN, OUTPUT);
	// pinMode(LED1_RED, OUTPUT);
	pinMode(LED2_GRN, OUTPUT);
	pinMode(LED2_RED, OUTPUT);
	pinMode(LED3_GRN, OUTPUT);
	pinMode(LED3_RED, OUTPUT);

	digitalWrite(LED1_GRN, LOW);
	// digitalWrite(LED1_RED, HIGH);
	digitalWrite(LED2_GRN, LOW);
	digitalWrite(LED2_RED, LOW);
	digitalWrite(LED3_GRN, LOW);
	digitalWrite(LED3_RED, LOW);

	// Điều khiển led trạng thái decawave
	if (decawave_ready_f == led_ok_DW)
	{
		this->decawace(led_ok_DW);
	}
	else
	{
		this->decawace(led_error_DW);
	}
	// Điều khiển led trạng thái internet
	if (internet_ready_f != led_internet_fail)
	{
		this->internet(internet_ready_f);
	}
	else
	{
		this->internet(led_internet_fail);
	}
	// Điều khiển led trạng thái nguồn
	if (power_ready_f == ledP_on)
	{
		this->power(ledP_on);
	}
	else
	{
		this->power(ledP_blink);
	}

	// this->decawace(led_error_DW);
	// this->internet(led_internet_fail);
	// this->power(ledP_blink);
}

void handle_status ::off_all(void)
{
	digitalWrite(LED1_GRN, LOW);
	// digitalWrite(LED1_RED, HIGH);
	digitalWrite(LED2_GRN, LOW);
	digitalWrite(LED2_RED, LOW);
	digitalWrite(LED3_GRN, LOW);
	digitalWrite(LED3_RED, LOW);
}

void handle_status::loop(void)
{
	// Bật chế độ nhấp nháy cho các LED
	InternetG.blink();
	InternetR.blink();
	DecawaceG.blink();
	DecawaceR.blink();
	PowerG.blink();

	// static unsigned long lastOkTime = 0; // Lưu thời gian cuối cùng điều kiện OK được thỏa mãn

	// // Kiểm tra nếu Internet, Decawace và Power đều ở trạng thái OK
	// if (internet_ready_f == led_internet_ok && decawave_ready_f == led_ok_DW)
	// {
	// 	// Nếu điều kiện OK, lưu thời gian hiện tại
	// 	if (lastOkTime == 0)
	// 	{
	// 		lastOkTime = millis();
	// 	}

	// 	// Kiểm tra nếu đã qua 30 giây (30000 ms)
	// 	if (millis() - lastOkTime >= 30000)
	// 	{
	// 		// Tắt tất cả LED
	// 		InternetG.blinkOff();
	// 		InternetR.blinkOff();
	// 		DecawaceG.blinkOff();
	// 		DecawaceR.blinkOff();
	// 		PowerG.blinkOff();
	// 		// PowerR.blinkOff();
	// 		// Serial.println("OFF ALL LED");
	// 	}
	// }
	// else
	// {
	// 	// Reset bộ đếm thời gian nếu điều kiện không thỏa mãn
	// 	lastOkTime = 0;

	// 	// Bật chế độ nhấp nháy cho các LED
	// 	InternetG.blink();
	// 	InternetR.blink();
	// 	DecawaceG.blink();
	// 	DecawaceR.blink();
	// 	PowerG.blink();
	// 	// PowerR.blink();
	// }
}

void handle_status::internet(status_ledINTERNET status)
{
	internet_ready_f = status;
	if (flag_timeout_led == true)
	{
		if (!stat_btn)
			return; // nếu nút nhấn không được bật thì không hiển thị led trạng thái internet
	}

	if (status == led_internet_fail)
	{
		InternetR.blinkOff();
		InternetG.blinkOff();
		InternetG.blink(500, 500);
	}
	else if (status == led_internet_ok)
	{
		InternetR.blinkOff();
		InternetG.blinkOff();
		InternetG.setBlinkState(BLINK_ON);
	}

	else if (status == led_fail_server)
	{
		InternetR.blinkOff();
		InternetR.setBlinkState(BLINK_ON);
		InternetG.blinkOff();
		InternetG.setBlinkState(BLINK_ON);
	}
	else if (status == led_fail_TCP)
	{
		InternetR.blinkOff();
		InternetR.setBlinkState(BLINK_ON);
		InternetG.blinkOff();
	}
	else if (status == led_fail_MQTT)
	{
		InternetR.blinkOff();
		InternetR.blink(500, 500);
		InternetG.blinkOff();
	}
}

void handle_status::decawace(status_ledDW status)
{
	decawave_ready_f = status;

	if (flag_timeout_led == true)
	{
		if (!stat_btn)
			return; // nếu nút nhấn không được bật thì không hiển thị led trạng thái decawave
	}

	if (status == led_error_DW)
	{
		DecawaceG.blinkOff();
		DecawaceR.blinkOff();
		DecawaceR.setBlinkState(BLINK_ON);
	}
	else if (status == led_ok_DW)
	{
		DecawaceR.blinkOff();
		DecawaceG.blinkOff();
		DecawaceG.setBlinkState(BLINK_ON);
	}
	else if (status == led_sync_ERROR_DW)
	{
		DecawaceR.blinkOff();
		DecawaceG.blinkOff();
		DecawaceG.blink(500, 500);
	}
	else if (status == led_clock_error_DW)
	{
		DecawaceR.blinkOff();
		DecawaceR.blink(500, 500);
		DecawaceG.blinkOff();
	}
}

void handle_status::power(status_ledPower status)
{
	power_ready_f = status;

	if (flag_timeout_led == true)
	{
		if (!stat_btn)
			return; // nếu nút nhấn không được bật thì không hiển thị led trạng thái nguồn
	}

	if (status == ledP_off)
	{
		// PowerR.blinkOff();
		PowerG.blinkOff();
	}
	else if (status == ledP_on)
	{
		// PowerR.blinkOff();
		PowerG.blinkOff();
		PowerG.setBlinkState(BLINK_ON);
	}
	else if (status == ledP_blink)
	{
		// PowerR.blinkOff();
		PowerG.blinkOff();
		PowerG.blink(500, 500);
	}
}

handle_status led_status;
