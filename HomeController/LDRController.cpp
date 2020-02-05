#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

#include "LDRController.h"


//REGISTER_CONTROLLER(LDRController)
REGISTER_CONTROLLER_FACTORY(LDRController)

const size_t bufferSize = JSON_OBJECT_SIZE(20);


#define BUF_SIZE_LDR  JSON_OBJECT_SIZE(20)
LDRController::LDRController() {
#ifdef  LDRCONTROLLER_DEBUG
	DBG_OUTPUT_PORT.println("LDRController Ctor");
#endif
	this->pin = 0;
	this->cvalmin = 0.0;
	this->cvalmax = MAX_LDRVAL;
#ifdef	ENABLE_NATIVE_HAP
	this->ishap = true;
	this->hapservice = NULL;
	this->hap_level = NULL;
	this->hapservice_type = "light";
	this->meassure_num = 5;
	//this->accessory_type = 0;
#endif

}
String  LDRController::serializestate() {

	DynamicJsonDocument jsonBuffer(bufferSize);
	JsonObject root = jsonBuffer.to<JsonObject>();
	root[FPSTR(szisOnText)] = this->get_state().isOn;
	root["ldrValue"] = this->get_state().ldrValue;

	if (cvalmin != cvalmax) {
		//float cval = map_i_f(this->get_state().ldrValue, 0, MAX_LDRVAL, cvalmin, cvalmax);
		root["cValue"] = this->get_state().cvalue;
		if (cfmt.length()) {
			char buff[30];
			snprintf(buff, sizeof(buff), cfmt.c_str(), this->get_state().cvalue);
			root["csValue"] = buff;
		}
	}
	String json;
	json.reserve(128);
	serializeJson(root, json);

	return json;
}
bool  LDRController::deserializestate(String jsonstate, CmdSource src) {

	DynamicJsonDocument jsonBuffer(bufferSize);
	DeserializationError error = deserializeJson(jsonBuffer, jsonstate);
	if (error) {
		DBG_OUTPUT_PORT.print(FPSTR(szParseJsonFailText));
		DBG_OUTPUT_PORT.println(this->get_name());
		DBG_OUTPUT_PORT.println(error.c_str());
		return false;
	}
	JsonObject root = jsonBuffer.as<JsonObject>();
	LDRState newState;
	newState.isOn = root[FPSTR(szisOnText)];
	newState.ldrValue = root["ldrValue"];
	
	this->AddCommand(newState, Measure, srcSelf);
	//this->set_state(newState);
	return true;

}
void LDRController::loadconfig(JsonObject& json) {
#ifdef  LDRCONTROLLER_DEBUG
	DBG_OUTPUT_PORT.println("LDRController before  load config");
#endif
	pin = json[FPSTR(szPinText)];
	cvalmin= json["cvalmin"].as<float>();
	cvalmax = json["cvalmax"].as<float>();
	cfmt = json["cfmt"].as<String>();
	loadif(hapservice_type, json, "haptype");


#ifdef  LDRCONTROLLER_DEBUG
	DBG_OUTPUT_PORT.println("LDRController load config");
#endif
}
void LDRController::getdefaultconfig(JsonObject& json) {
	json[FPSTR(szPinText)] = pin;
	json[FPSTR(szservice)] = "LDRController";
	json[FPSTR(szname)] = "LDR";
	json["cvalmin"]= cvalmin;
	json["cvalmax"]= cvalmax;
	json["cfmt"] = cfmt;
	json["haptype"] = hapservice_type;
	LDR::getdefaultconfig(json);
}
void  LDRController::setup() {
	pinMode(pin, INPUT);
	//digitalWrite(pin, LOW);
}

void LDRController::run() {
	if (this->commands.GetSize() == 0) {
		command newcmd;
		newcmd.mode = Measure;
		newcmd.state.ldrValue = meassure();// analogRead(pin);
#ifdef  LDRCONTROLLER_DEBUG
		DBG_OUTPUT_PORT.print("LDR ctl run value->");
		DBG_OUTPUT_PORT.println(newcmd.state.ldrValue);
#endif //  LDRCONTROLLER_DEBUG

		//this->commands.Add(newcmd);
		this->AddCommand(newcmd.state, newcmd.mode, srcSelf);
	}
	command cmd;
	while (commands.Dequeue(&cmd)) {
		if (this->baseprocesscommands(cmd))
			continue;
		LDRState newState = cmd.state;

		this->set_state(newState);
	}
	LDR::run();
}
uint16_t LDRController::meassure() {
	int accres=0;
	for (int i = 0; i < meassure_num; i++) {
		accres+= analogRead(pin);
		delay(2);
	}
	return accres / meassure_num;
}
void LDRController::set_state(LDRState state) {
	if (cvalmin != cvalmax)
		state.cvalue = map_i_f(state.ldrValue, 0, MAX_LDRVAL, cvalmin, cvalmax);
	else
		state.cvalue = state.ldrValue;
	LDR::set_state(state);
	
}

bool LDRController::onpublishmqtt(String& endkey, String& payload) {
	endkey = szStatusText;
	payload = String(this->get_state().ldrValue);
	return true;
}

#ifdef	ENABLE_NATIVE_HAP
void LDRController::setup_hap_service() {
	

	//DBG_OUTPUT_PORT.println("LDRController::setup_hap_service()");
	if (!ishap) {
		
		//DBG_OUTPUT_PORT.println("LDRController::NO Hap support");
		return;
	}
	//DBG_OUTPUT_PORT.println(this->hapservice_type);
//	DBG_OUTPUT_PORT.println("LDRController ishap");
	//DBG_OUTPUT_PORT.println(hapservice_type);
	if (this->hapservice_type == "light") {
		//DBG_OUTPUT_PORT.println("LDRController  light");
		if (this->accessory_type > 1) {
			DBG_OUTPUT_PORT.println("LDRController  accessory_type > 1");
			this->hapservice = hap_add_light_service_as_accessory(this->accessory_type, this->get_name(), LDRController::hap_callback, this);
		}
		else
		{
			this->hapservice = hap_add_light_service(this->get_name(), LDRController::hap_callback, this);
		}
		this->hap_level= homekit_service_characteristic_by_type(this->hapservice, HOMEKIT_CHARACTERISTIC_CURRENT_AMBIENT_LIGHT_LEVEL);
	}
	if (this->hapservice_type == "bat") {
		//DBG_OUTPUT_PORT.println("LDRController  bat");
		this->hapservice = hap_add_battery_service(this->get_name(), LDRController::hap_callback, this);
		this->hap_level = homekit_service_characteristic_by_type(this->hapservice, HOMEKIT_CHARACTERISTIC_BATTERY_LEVEL);
	}
	
}
void LDRController::notify_hap() {
	if (this->ishap && this->hapservice) {
		//DBG_OUTPUT_PORT.println("LDRController::notify_hap");
		LDRState lstate = this->get_state();
		if (this->hapservice_type == "light" && this->hap_level) {
			if (this->hap_level->value.float_value != lstate.cvalue) {
				this->hap_level->value.float_value = lstate.cvalue;
				homekit_characteristic_notify(this->hap_level, this->hap_level->value);
			}
		}
		if (this->hapservice_type == "bat" && this->hap_level) {
			float chargepercent = 100.0*((cvalmax + cvalmin) / 2.0) / lstate.cvalue;
			if (this->hap_level->value.float_value != chargepercent) {
				this->hap_level->value.float_value = chargepercent;
				homekit_characteristic_notify(this->hap_level, this->hap_level->value);
			}
			homekit_characteristic_t* hc_charg = homekit_service_characteristic_by_type(this->hapservice, HOMEKIT_CHARACTERISTIC_CHARGING_STATE);
			homekit_characteristic_t* hc_lb = homekit_service_characteristic_by_type(this->hapservice, HOMEKIT_CHARACTERISTIC_STATUS_LOW_BATTERY);

			HAP_NOTIFY_CHANGES(bool, hc_charg, false, 0);

			if (hc_lb && cvalmin != cvalmax) {
				bool blowlevel = lstate.cvalue < (cvalmax*2.0 / 3.0);

			}
		}
	}
}
void LDRController::hap_callback(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
	DBG_OUTPUT_PORT.println("RGBStripController::hap_callback");

	if (!context) {
		return;
	};

	LDRController* ctl = (LDRController*)context;

	/*
	if (ch == ctl->hap_br && ch->value.int_value != newState.brightness) {
		newState.brightness = ch->value.int_value;
		cmd = DimSetBrigthness;
		isSet = true;
	}

	//	newState.isOn=value.bool_value;
	if (isSet)
		ctl->AddCommand(newState, cmd, srcHAP);
		*/

}
#endif