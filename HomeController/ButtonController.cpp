#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

#include "ButtonController.h"

//#ifndef ESP8266   /// further solving factory
//REGISTER_CONTROLLER(ButtonController)
//#endif // !1
#ifndef DISABLE_BUTTON
REGISTER_CONTROLLER_FACTORY(ButtonController)
#endif
const size_t bufferSize = JSON_OBJECT_SIZE(10);

ButtonController::ButtonController() {
	memset(pin, 0, sizeof(uint8_t)*MAX_BUTTONS); 
	memset(btnhistory, 0, sizeof(uint16_t)*MAX_BUTTONS);
	memset(btnstate, 0, sizeof(enumstate)*MAX_BUTTONS);

	memset(btn_long_history, 0, sizeof(btn_state_history_record)*STATE_HISTORY_MAX*MAX_BUTTONS);

	this->btncount = 0;
	this->defstate = true;
	Ticker* pTicker;
	this->coreMode = Both;
	this->last_history_state_update = 0;
}

String  ButtonController::serializestate() {

	DynamicJsonDocument jsonBuffer(bufferSize);
	JsonObject root = jsonBuffer.to<JsonObject>();
	root["isPressed"] = this->get_state().isPressed;

	String json;
	json.reserve(64);
	serializeJson(root, json);

	return json;
}
bool  ButtonController::deserializestate(String jsonstate, CmdSource src) {

	DynamicJsonDocument jsonBuffer(bufferSize);
	DeserializationError error = deserializeJson(jsonBuffer, jsonstate);
	if (error) {
		DBG_OUTPUT_PORT.print(FPSTR(szParseJsonFailText));
		DBG_OUTPUT_PORT.println(this->get_name());
		DBG_OUTPUT_PORT.println(error.c_str());
		return false;
	}
	JsonObject root = jsonBuffer.as<JsonObject>();
	ButtonState newState;
	newState.isPressed = root["isPressed"];


//	this->AddCommand(newState, Measure, src);
	//this->set_state(newState);
	return true;

}
void ButtonController::loadconfig(JsonObject& json) {
	uint8_t mpin = json[FPSTR(szPinText)];
	JsonArray arr = json["pins"].as<JsonArray>();
	loadif(this->defstate, json, "defstate");
#ifdef BUTTON_DEBUG
	DBG_OUTPUT_PORT.println(F("Button setup pins"));
#endif
	if (mpin != 0) {
#ifdef BUTTON_DEBUG
		DBG_OUTPUT_PORT.println(F("Single"));
		DBG_OUTPUT_PORT.println(mpin);
#endif
		this->btncount = 1;
		this->pin[0] = mpin;
	}
	else {
		this->btncount = constrain(arr.size(), 0, MAX_BUTTONS);
		for (int i = 0; i < this->btncount; i++) {
			this->pin[i] = arr[i];
#ifdef BUTTON_DEBUG
			DBG_OUTPUT_PORT.println(this->pin[i]);
#endif

		}
	}
	
}
//The five buttons are controlled by FLASH, RESET, D5, D6, and D7 respectively.
/*
static const uint8_t D0 = 16;
static const uint8_t D1 = 5;
static const uint8_t D2 = 4;
static const uint8_t D3 = 0;
static const uint8_t D4 = 2;
static const uint8_t D5 = 14;
static const uint8_t D6 = 12;
static const uint8_t D7 = 13;
static const uint8_t D8 = 15;
static const uint8_t RX = 3;
static const uint8_t TX = 1;
*/
void ButtonController::getdefaultconfig(JsonObject& json) {
	json[FPSTR(szPinText)] = pin;
	json[FPSTR(szservice)] = "ButtonController";
	json[FPSTR(szname)] = "Button";
	json["pins"] = "";
	
	Button::getdefaultconfig(json);
}

void  ButtonController::setup() {
	Button::setup();
	for(uint8_t i=0;i<btncount;i++)
		pinMode(pin[i], INPUT_PULLUP);
	//digitalWrite(pin, LOW);
}
void ButtonController::run() {

	command cmd;
	while (commands.Dequeue(&cmd)) {
		if (this->baseprocesscommands(cmd))
			continue;
#ifdef BUTTON_DEBUG
		DBG_OUTPUT_PORT.println(F("button set state"));
		DBG_OUTPUT_PORT.println(this->events.GetSize());
#endif
		this->set_state(cmd.state);
	}
}
void ButtonController::runcore() {
	for (uint8_t i = 0;i < btncount;i++) {
		this->update(i);
		ButtonState st;
		st.idx = i;
		if (this->is_down(i) && btnstate[i]!= isdown){  //this->get_state().isDown) {
			st.isDown = !this->get_state().isDown;
		    btnstate[i] = isdown;
			st.changed_at = millis();
			this->AddCommand(st, SetBtn, srcSelf);
			this->update_history_state(i, btnstate[i], millis());
#ifdef BUTTON_DEBUG
			DBG_OUTPUT_PORT.println(F("button down"));
#endif
		}

		if (this->is_pressed(i) && (btnstate[i] != ispressed && btnstate[i] != islongpressed)){//!= this->get_state(i).isPressed) {
			st.changed_at = btnpresstime[i] =millis();

			longhistoryres res = this->update_history_state(i, ispressed, millis());
			
			if (res == his_longpressed) {
				btnstate[i] = islongpressed;
				st.isLongPressed = true;
				st.isPressed = false;
			}
			else {
				st.isPressed = true;
				st.isLongPressed = false;
				st.changed_at = btnpresstime[i] = millis();
				btnstate[i] = ispressed;
			}
			this->AddCommand(st, SetBtn, srcSelf);
			
			
#ifdef BUTTON_DEBUG
			DBG_OUTPUT_PORT.println(F("button press"));
#endif
		}
	}
	//if ((last_history_state_update + STATE_HISTORY_PERIOD_UPDATE) < millis()) {
	//	for (uint8_t i = 0;i < btncount;i++)
	//		check_update_longhistory(i);
		
	//}
}
longhistoryres ButtonController::update_history_state(uint8_t idx, enumstate state, long ms) {

	//shifting left
	memcpy(btn_long_history[idx], btn_long_history[idx] + 1, (sizeof(btn_state_history_record)*(STATE_HISTORY_MAX-1)));
	//btn_state_history_record last= btn_long_history[idx][STATE_HISTORY_MAX - 1];
	btn_long_history[idx][STATE_HISTORY_MAX - 1].state = state;
	btn_long_history[idx][STATE_HISTORY_MAX - 1].ms = ms;
	//last.state = state;
	//last.ms = ms;
	return check_update_longhistory(idx);
}
longhistoryres ButtonController::check_update_longhistory(uint8_t idx) {
	last_history_state_update = millis();
	btn_state_history_record last = btn_long_history[idx][STATE_HISTORY_MAX - 1];
	int presscount = 0;
	if (last.state == ispressed) {
		long lastpresstime = last.ms;
		long durationpress = 0;
		btn_state_history_record prev = btn_long_history[idx][STATE_HISTORY_MAX - 2];
		//if (prev.state == isdown) {
		//	DBG_OUTPUT_PORT.println(String("isdown ")+ String(last.ms)+String(" ")+ String(prev.ms) + String(" ") + String(last.ms-prev.ms));
		//}
		if (prev.state == isdown && ((last.ms - prev.ms) > LONG_ACTIONDURATION)) {
			return his_longpressed;
		}
		for (int i = STATE_HISTORY_MAX - 2; i >= 0; i--) {
			btn_state_history_record it = btn_long_history[idx][i];
			//DBG_OUTPUT_PORT.println(String(i)+ String(" ")+String(it.state)+ String(" ")+ String(it.ms));
			if (it.state == ispressed) {

				lastpresstime = it.ms;
				presscount++;
				//}else
				//	durationpress = lastpresstime-it.ms;
				//	if (durationpress > LONG_ACTIONDURATION) {
				//		return his_longpressed;
				//	}
			}
		}
	}
	if (presscount > 1)
		return his_multiplepress;
	return his_none;
	/*
	if (last.state != ispressed &&
		btn_long_history[idx][STATE_HISTORY_MAX - 2].state == ispressed &&
		(btn_long_history[idx][STATE_HISTORY_MAX - 2].ms + LONG_ACTIONDURATION) < last.ms) {
		ButtonState st = this->get_state();
		st.idx = idx;
		st.isLongPressed = true;
		this->AddCommand(st, SetBtn, srcSelf);
#ifdef BUTTON_DEBUG
		DBG_OUTPUT_PORT.println(F("button long press"));
#endif
	}
	*/
	

}
void ButtonController::update(uint8_t idx) {

	this->addhistory(digitalRead(pin[idx])!=(this->defstate?HIGH:LOW),idx);
}
void ButtonController::addhistory(bool bit, uint8_t idx) {

	this->btnhistory[idx] = (this->btnhistory[idx] << 1) | bit;
}
bool  ButtonController::is_down(uint8_t idx) {
	return ((this->btnhistory[idx] & ADJUST_MASK) == DOWN_MASK);
}
bool  ButtonController::is_pressed(uint8_t idx) {
	return ((this->btnhistory[idx] & ADJUST_MASK) == PRESSED_MASK);
}