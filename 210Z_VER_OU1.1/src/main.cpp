/**
 * @file main.cpp
 * @author Zechariah Wang
 * @brief Main logic file. Runs pre-init, init, auton, and op-control logic
 * @version 0.1
 * @date 2023-07-04
 */

#include "main.h"
#include "210Z-Lib/Chassis/AbsolutePositioningSystemController.hpp"
#include "Config/Globals.hpp"
#include "display/lv_objx/lv_label.h"
#include "pros/adi.hpp"
#include "pros/misc.h"
#include "pros/motors.h"
#include "vector"
#include "variant"
#include "array"
#include "map"
#include "string"
#include <type_traits>
#include "210Z-Lib/Modules/UtilityModule.hpp"

using namespace Eclipse;

constexpr u_int64_t time_dt = 100000; // Time until initialize phase ends. Effectively infinite.
constexpr u_int16_t delayAmount = 10; // Dont overload the CPU during OP control
char buffer[100];

// // Chassis drivetrain config. If you want to config sensors, and misc subsystems go to globals.cpp
// AssetConfig config(
// 	{5, 21, 4, 3}, // Left Motor Ports (negative value means ports are reversed)
// 	{-13, -20, -14, -12} // Right Motor Ports (negative value means port is reversed)
// ); 

// Chassis drivetrain config. If you want to config sensors, and misc subsystems go to globals.cpp
AssetConfig config(
	{-14, -15, -13, -12}, // Left Motor Ports (negative value means ports are reversed)
	{17, 16, 18, 19} // Right Motor Ports (negative value means port is reversed)
); 


// sarah
// AssetConfig config(
// 	{-10, -9, -190}, // Left Motor Ports (negative value means ports are reversed)
// 	{20, 11, 80} // Right Motor Ports (negative value means port is reversed)
// ); 

pros::ADIEncoder vertical_auxiliary_sensor('y', 'z', true); // vertical tracking wheel
pros::Rotation horizontal_rotation_sensor(19); // horizontal tracking wheel
pros::Imu imu_sensor(1); // IMU sensor

// Game specific subsystems. Header declaration is in globals.hpp
pros::ADIDigitalIn cata_sensor('o');
pros::Distance distance_sensor(-22);
pros::Motor cata_motor(9, pros::E_MOTOR_GEARSET_06, false, pros::E_MOTOR_ENCODER_COUNTS); // flywheel
pros::Motor flywheel_arm(17, pros::E_MOTOR_GEARSET_06, false, pros::E_MOTOR_ENCODER_COUNTS); 
pros::Motor cata_motor_secondary(109, pros::E_MOTOR_GEARSET_36, true, pros::E_MOTOR_ENCODER_COUNTS);
pros::Motor intake_motor(13, pros::E_MOTOR_GEARSET_06, true, pros::E_MOTOR_ENCODER_COUNTS);
pros::ADIDigitalOut climber('h');
pros::ADIDigitalOut primary_climber('e');

pros::ADIDigitalOut left_wing('g');
pros::ADIDigitalOut right_wing('c');
pros::ADIDigitalOut left_front_wing('f');
pros::ADIDigitalOut right_front_wing('b');
pros::ADIDigitalOut odom_piston('a');

// not used
pros::ADIDigitalOut blocker('z');
pros::ADIDigitalOut front_wings('z');

lv_obj_t *sensor_button_home; lv_obj_t *auton_button_home; lv_obj_t *misc_button_home; lv_obj_t *game_button_home; lv_obj_t *welcomeDisplay; lv_obj_t *home_welcome_text; lv_obj_t *home_page = lv_page_create(lv_scr_act(), NULL);
lv_obj_t *odom_readings_sensor; lv_obj_t *dt_readings_sensor; lv_obj_t *sensor1_readings_sensor; lv_obj_t *sensor2_readings_sensor; lv_obj_t *sensor3_readings_sensor; lv_obj_t *sensor4_readings_sensor; lv_obj_t *return_button_sensor;
lv_obj_t *current_auton_display_selector; lv_obj_t *prev_auton_button_selector; lv_obj_t *next_auton_button_selector; lv_obj_t *select_auton_button_selector; lv_obj_t *return_auton_button_selector;
lv_obj_t *controller_status_game; lv_obj_t *battery_percent_game; lv_obj_t *battery_temp_game; lv_obj_t *time_since_startup_game; lv_obj_t *competition_stat_game; lv_obj_t *return_button_game;
lv_obj_t *debug_line1_misc; lv_obj_t *debug_line2_misc; lv_obj_t *debug_line3_misc; lv_obj_t *debug_line4_misc; lv_obj_t *debug_line5_misc;
lv_obj_t *debug_line6_misc; lv_obj_t *debug_line7_misc; lv_obj_t *debug_line8_misc; lv_obj_t *debug_line9_misc; lv_obj_t *debug_line10_misc; lv_obj_t *return_button_misc;
lv_obj_t *displayDataL1; lv_obj_t *displayDataL2; lv_obj_t *displayDataL3; lv_obj_t *displayDataL4; lv_obj_t *displayDataL5; lv_obj_t *debugLine1; lv_obj_t *debugLine2;

std::map<int, std::string> auton_Legend = {
    { 1, "Win Point" },
    { 2, "LS Priority: WP" },
    { 3, "LS Priority: Secondary Auton" },
	{ 4, "RS Priority: Six Ball " },
    { 5, "RS Priority: Secondary Auton" },
    { 6, "Empty Slot" },
    { 7, "Empty Slot" },
    { 8, "Empty Slot" },
    { 9, "Empty Slot" },
    { 10,"Empty Slot" }
};

void display_homePage()
{
	lv_obj_set_hidden(sensor_button_home, false);  
	lv_obj_set_hidden(auton_button_home, false);
	lv_obj_set_hidden(misc_button_home, false);
	lv_obj_set_hidden(game_button_home, false);
	lv_obj_set_hidden(home_welcome_text, false);

	lv_obj_set_hidden(odom_readings_sensor, true);  
	lv_obj_set_hidden(dt_readings_sensor, true);
	lv_obj_set_hidden(sensor1_readings_sensor, true);
	lv_obj_set_hidden(sensor2_readings_sensor, true);
	lv_obj_set_hidden(sensor3_readings_sensor, true);
	lv_obj_set_hidden(sensor4_readings_sensor, true);
	lv_obj_set_hidden(return_button_sensor, true);

	lv_obj_set_hidden(current_auton_display_selector, true);  
	lv_obj_set_hidden(next_auton_button_selector, true);
	lv_obj_set_hidden(select_auton_button_selector, true);
	lv_obj_set_hidden(prev_auton_button_selector, true);
	lv_obj_set_hidden(return_auton_button_selector, true);
}

void display_sensorPage()
{
	lv_obj_set_hidden(odom_readings_sensor, false);  
	lv_obj_set_hidden(dt_readings_sensor, false);
	lv_obj_set_hidden(sensor1_readings_sensor, false);
	lv_obj_set_hidden(sensor2_readings_sensor, false);
	lv_obj_set_hidden(sensor3_readings_sensor, false);
	lv_obj_set_hidden(sensor4_readings_sensor, false);
	lv_obj_set_hidden(return_button_sensor, false);
}

void display_autonPage()
{
	lv_obj_set_hidden(current_auton_display_selector, false);  
	lv_obj_set_hidden(next_auton_button_selector, false);
	lv_obj_set_hidden(prev_auton_button_selector, false);
	lv_obj_set_hidden(return_auton_button_selector, false);
	lv_obj_set_hidden(select_auton_button_selector, false);
}

void display_gamePage()
{
	lv_obj_set_hidden(controller_status_game, false);  
	lv_obj_set_hidden(battery_percent_game, false);
	lv_obj_set_hidden(battery_temp_game, false);
	lv_obj_set_hidden(time_since_startup_game, false);
	lv_obj_set_hidden(competition_stat_game, false);
	lv_obj_set_hidden(return_button_game, false);
}

void display_miscPage()
{
	lv_obj_set_hidden(debug_line1_misc, false);  
	lv_obj_set_hidden(debug_line2_misc, false);
	lv_obj_set_hidden(debug_line3_misc, false);
	lv_obj_set_hidden(debug_line4_misc, false);
	lv_obj_set_hidden(debug_line5_misc, false);
	lv_obj_set_hidden(debug_line6_misc, false);
	lv_obj_set_hidden(return_button_misc, false);
}

void hide_homePage()
{
	lv_obj_set_hidden(sensor_button_home, true);  
	lv_obj_set_hidden(auton_button_home, true);
	lv_obj_set_hidden(misc_button_home, true);
	lv_obj_set_hidden(game_button_home, true);
	lv_obj_set_hidden(home_welcome_text, true);
}

void hide_sensorPage()
{
	lv_obj_set_hidden(odom_readings_sensor, true);  
	lv_obj_set_hidden(dt_readings_sensor, true);
	lv_obj_set_hidden(sensor1_readings_sensor, true);
	lv_obj_set_hidden(sensor2_readings_sensor, true);
	lv_obj_set_hidden(sensor3_readings_sensor, true);
	lv_obj_set_hidden(sensor4_readings_sensor, true);
	lv_obj_set_hidden(return_button_sensor, true);
}

void hide_autonPage()
{
	lv_obj_set_hidden(current_auton_display_selector, true);  
	lv_obj_set_hidden(next_auton_button_selector, true);
	lv_obj_set_hidden(select_auton_button_selector, true);
	lv_obj_set_hidden(prev_auton_button_selector, true);
	lv_obj_set_hidden(return_auton_button_selector, true);
}

void hide_gamePage()
{
	lv_obj_set_hidden(controller_status_game, true);  
	lv_obj_set_hidden(battery_percent_game, true);
	lv_obj_set_hidden(battery_temp_game, true);
	lv_obj_set_hidden(time_since_startup_game, true);
	lv_obj_set_hidden(competition_stat_game, true);
	lv_obj_set_hidden(return_button_game, true);
}

void hide_miscPage()
{
	lv_obj_set_hidden(debug_line1_misc, true);  
	lv_obj_set_hidden(debug_line2_misc, true);
	lv_obj_set_hidden(debug_line3_misc, true);
	lv_obj_set_hidden(debug_line4_misc, true);
	lv_obj_set_hidden(debug_line5_misc, true);
	lv_obj_set_hidden(debug_line6_misc, true);
	lv_obj_set_hidden(return_button_misc, true);
}

static lv_res_t return_to_home(lv_obj_t *btn){
	lv_obj_set_hidden(sensor_button_home, false);  
	lv_obj_set_hidden(auton_button_home, false);
	lv_obj_set_hidden(misc_button_home, false);
	lv_obj_set_hidden(game_button_home, false);
	lv_obj_set_hidden(home_welcome_text, false);
	lv_obj_set_hidden(odom_readings_sensor, true); // sensor
	lv_obj_set_hidden(dt_readings_sensor, true);
	lv_obj_set_hidden(sensor1_readings_sensor, true);
	lv_obj_set_hidden(sensor2_readings_sensor, true);
	lv_obj_set_hidden(sensor3_readings_sensor, true);
	lv_obj_set_hidden(sensor4_readings_sensor, true);
	lv_obj_set_hidden(return_button_sensor, true);
	lv_obj_set_hidden(current_auton_display_selector, true); // auton selector
	lv_obj_set_hidden(next_auton_button_selector, true);
	lv_obj_set_hidden(current_auton_display_selector, true);
	lv_obj_set_hidden(select_auton_button_selector, true);
	lv_obj_set_hidden(prev_auton_button_selector, true);
	lv_obj_set_hidden(return_auton_button_selector, true);
	lv_obj_set_hidden(controller_status_game, true); // game 
	lv_obj_set_hidden(battery_percent_game, true);
	lv_obj_set_hidden(battery_temp_game, true);
	lv_obj_set_hidden(time_since_startup_game, true);
	lv_obj_set_hidden(competition_stat_game, true);
	lv_obj_set_hidden(return_button_game, true);
	lv_obj_set_hidden(debug_line1_misc, true); // misc
	lv_obj_set_hidden(debug_line2_misc, true);
	lv_obj_set_hidden(debug_line3_misc, true);
	lv_obj_set_hidden(debug_line4_misc, true);
	lv_obj_set_hidden(debug_line5_misc, true);
	lv_obj_set_hidden(debug_line6_misc, true);
	lv_obj_set_hidden(return_button_misc, true);
	return 0;
}

static lv_res_t sensor_on_click(lv_obj_t *btn){
	hide_homePage();
	hide_autonPage();
	hide_gamePage();
	hide_miscPage();
	display_sensorPage();
	return 0;
}

static lv_res_t auton_on_click(lv_obj_t *btn){
	hide_homePage();
	hide_sensorPage();
	hide_gamePage();
	hide_miscPage();
	display_autonPage();
	return 0;
}

static lv_res_t misc_on_click(lv_obj_t *btn){
	hide_homePage();
	hide_sensorPage();
	hide_gamePage();
	hide_autonPage();
	display_miscPage();
	return 0;
}

static lv_res_t game_on_click(lv_obj_t *btn){
	hide_homePage();
	hide_sensorPage();
	hide_autonPage();
	hide_miscPage();
	display_gamePage();
	return 0;
}

//-- LVGL on input functions //--
static lv_res_t selectAuton(lv_obj_t *btn){
	static bool pressed = true;
	if (pressed) { auton_finalized = 1; } 
	return 0;
}

//-- LVGL prev auton selector functions
static lv_res_t onPrevPress(lv_obj_t *btn){
    selected_auton -= 1;
    if (selected_auton > 10){
        selected_auton = 1;
		sprintf(buffer, SYMBOL_LIST " Selected Path %d: %s", selected_auton, auton_Legend[selected_auton].c_str());
        lv_label_set_text(current_auton_display_selector, buffer);
    }
    else if (selected_auton < 1){
        selected_auton = 10;
		sprintf(buffer, SYMBOL_LIST " Selected Path %d: %s", selected_auton, auton_Legend[selected_auton].c_str());
        lv_label_set_text(current_auton_display_selector, buffer);
	}
	sprintf(buffer, SYMBOL_LIST " Selected Path %d: %s", selected_auton, auton_Legend[selected_auton].c_str());
    lv_label_set_text(current_auton_display_selector, buffer);
	return 1;
}

//-- LVGL next auton selector functions
static lv_res_t onNextPress(lv_obj_t *btn){
	selected_auton += 1;
    if (selected_auton > 10){
        selected_auton = 1;
		sprintf(buffer, SYMBOL_LIST " Selected Path %d: %s", selected_auton, auton_Legend[selected_auton].c_str());
        lv_label_set_text(current_auton_display_selector, buffer);
    }
    else if (selected_auton < 1){
        selected_auton = 10;
		sprintf(buffer, SYMBOL_LIST " Selected Path %d: %s", selected_auton, auton_Legend[selected_auton].c_str());
        lv_label_set_text(debugLine1, buffer);
    }
	sprintf(buffer, SYMBOL_LIST " Selected Path %d: %s", selected_auton, auton_Legend[selected_auton].c_str());
    lv_label_set_text(current_auton_display_selector, buffer);
	return 1;
}

void initialize() { // Init function control

    static lv_style_t sensor_button_style;                         
    lv_style_copy(&sensor_button_style, &lv_style_pretty);                    
    sensor_button_style.body.main_color = LV_COLOR_MAKE(47, 144, 212);         
    sensor_button_style.body.grad_color = LV_COLOR_MAKE(47, 144, 212);
	sensor_button_style.body.radius = 8;                       
    sensor_button_style.text.color = LV_COLOR_WHITE;     

    static lv_style_t auton_button_style;                         
    lv_style_copy(&sensor_button_style, &lv_style_pretty);                    
    sensor_button_style.body.main_color = LV_COLOR_MAKE(47, 144, 212);         
    sensor_button_style.body.grad_color = LV_COLOR_MAKE(47, 144, 212);
	sensor_button_style.body.radius = 8;                       
    sensor_button_style.text.color = LV_COLOR_WHITE;  

    static lv_style_t misc_button_style;                         
    lv_style_copy(&sensor_button_style, &lv_style_pretty);                    
    sensor_button_style.body.main_color = LV_COLOR_MAKE(47, 144, 212);         
    sensor_button_style.body.grad_color = LV_COLOR_MAKE(47, 144, 212);
	sensor_button_style.body.radius = 8;                       
    sensor_button_style.text.color = LV_COLOR_WHITE;  

    static lv_style_t game_button_style;                         
    lv_style_copy(&sensor_button_style, &lv_style_pretty);                    
    sensor_button_style.body.main_color = LV_COLOR_MAKE(47, 144, 212);         
    sensor_button_style.body.grad_color = LV_COLOR_MAKE(47, 144, 212);
	sensor_button_style.body.radius = 8;                       
    sensor_button_style.text.color = LV_COLOR_WHITE;           

    static lv_style_t style_home_page;                         
    lv_style_copy(&style_home_page, &lv_style_pretty);              
    style_home_page.body.main_color = lv_color_hsv_to_rgb(0, 0, 7);   
    style_home_page.body.grad_color = lv_color_hsv_to_rgb(0, 0, 7);    
    style_home_page.body.border.width = 1;   
	style_home_page.text.color = LV_COLOR_WHITE; 
	lv_obj_set_size(home_page, 500, 300);
	lv_obj_set_style(home_page, &style_home_page);

	sensor_button_home = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(sensor_button_home, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(sensor_button_home, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(sensor_button_home, LV_BTN_ACTION_CLICK, sensor_on_click); 
    lv_obj_align(sensor_button_home, NULL, LV_ALIGN_CENTER, -100, -20);
	lv_obj_set_height(sensor_button_home, 60);
	lv_obj_set_width(sensor_button_home, 160);
    lv_obj_set_style(sensor_button_home, &sensor_button_style);
	lv_obj_t *sensor_buttonText = lv_label_create(home_page, NULL);
    lv_label_set_text(sensor_buttonText, "");  
    lv_obj_set_x(sensor_buttonText, 50); 
    sensor_buttonText = lv_label_create(sensor_button_home, NULL);
    lv_label_set_text(sensor_buttonText, SYMBOL_GPS " SENSOR");
	
	auton_button_home = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(auton_button_home, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(auton_button_home, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(auton_button_home, LV_BTN_ACTION_CLICK, auton_on_click); 
    lv_obj_align(auton_button_home, NULL, LV_ALIGN_CENTER, 70, -20);
	lv_obj_set_height(auton_button_home, 60);
	lv_obj_set_width(auton_button_home, 160);
    lv_obj_set_style(auton_button_home, &sensor_button_style);
	lv_obj_t *auton_buttonText = lv_label_create(home_page, NULL);
    lv_label_set_text(auton_buttonText, "");  
    lv_obj_set_x(auton_buttonText, 50); 
    auton_buttonText = lv_label_create(auton_button_home, NULL);
    lv_label_set_text(auton_buttonText, SYMBOL_DIRECTORY " AUTON");

	misc_button_home = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(misc_button_home, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(misc_button_home, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(misc_button_home, LV_BTN_ACTION_CLICK, misc_on_click); 
    lv_obj_align(misc_button_home, NULL, LV_ALIGN_CENTER, 70, 50);
	lv_obj_set_height(misc_button_home, 60);
	lv_obj_set_width(misc_button_home, 160);
    lv_obj_set_style(misc_button_home, &sensor_button_style);
	lv_obj_t *misc_buttonText = lv_label_create(home_page, NULL);
    lv_label_set_text(misc_buttonText, "");  
    lv_obj_set_x(misc_buttonText, 50); 
    misc_buttonText = lv_label_create(misc_button_home, NULL);
    lv_label_set_text(misc_buttonText, SYMBOL_COPY " MISC");

	game_button_home = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(game_button_home, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(game_button_home, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(game_button_home, LV_BTN_ACTION_CLICK, game_on_click); 
    lv_obj_align(game_button_home, NULL, LV_ALIGN_CENTER, -100, 50);
	lv_obj_set_height(game_button_home, 60);
	lv_obj_set_width(game_button_home, 160);
    lv_obj_set_style(game_button_home, &sensor_button_style);
	lv_obj_t *game_buttonText = lv_label_create(home_page, NULL);
    lv_label_set_text(game_buttonText, "");  
    lv_obj_set_x(game_buttonText, 50); 
    game_buttonText = lv_label_create(game_button_home, NULL);
    lv_label_set_text(game_buttonText, SYMBOL_CHARGE " GAME");

    home_welcome_text =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(home_welcome_text, "Welcome 210Z, ");
    lv_obj_align(home_welcome_text, NULL, LV_ALIGN_IN_LEFT_MID, 85, -80);

	// Sensor Page
    odom_readings_sensor =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(odom_readings_sensor, SYMBOL_GPS " X: 0.0 Y: 0.0 Theta: 0.0");
    lv_obj_align(odom_readings_sensor, NULL, LV_ALIGN_IN_LEFT_MID, 20, -75);

    dt_readings_sensor =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(dt_readings_sensor, SYMBOL_LIST " FL: 0.0 BL: 0.0 FL: 0.0 FR: 0.0");
    lv_obj_align(dt_readings_sensor, NULL, LV_ALIGN_IN_LEFT_MID, 20, -55);

    sensor1_readings_sensor =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(sensor1_readings_sensor, SYMBOL_REFRESH " Cata Pos: 0.0");
    lv_obj_align(sensor1_readings_sensor, NULL, LV_ALIGN_IN_LEFT_MID, 20, -35);

    sensor2_readings_sensor =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(sensor2_readings_sensor, SYMBOL_LOOP " Intake Status: 0.0");
    lv_obj_align(sensor2_readings_sensor, NULL, LV_ALIGN_IN_LEFT_MID, 20, -15);

    sensor3_readings_sensor =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(sensor3_readings_sensor, SYMBOL_IMAGE " Ultrasonic Status: 0.0");
    lv_obj_align(sensor3_readings_sensor, NULL, LV_ALIGN_IN_LEFT_MID, 20, 5);

    sensor4_readings_sensor =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(sensor4_readings_sensor, SYMBOL_EJECT "  Limit Status: 0.0");
    lv_obj_align(sensor4_readings_sensor, NULL, LV_ALIGN_IN_LEFT_MID, 20, 25);

	return_button_sensor = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(return_button_sensor, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(return_button_sensor, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(return_button_sensor, LV_BTN_ACTION_CLICK, return_to_home); 
    lv_obj_align(return_button_sensor, NULL, LV_ALIGN_CENTER, 100, 110);
	lv_obj_set_height(return_button_sensor, 30);
	lv_obj_set_width(return_button_sensor, 160);
    lv_obj_set_style(return_button_sensor, &sensor_button_style);
	lv_obj_t *return_buttonText = lv_label_create(home_page, NULL);
    lv_label_set_text(return_buttonText, "");  
    lv_obj_set_x(return_buttonText, 50); 
    return_buttonText = lv_label_create(return_button_sensor, NULL);
    lv_label_set_text(return_buttonText, SYMBOL_CLOSE " Return");

	// Auton page
    current_auton_display_selector =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(current_auton_display_selector, " Current Selected Path: Solo Win Point");
    lv_obj_align(current_auton_display_selector, NULL, LV_ALIGN_CENTER, -30, -30);

	prev_auton_button_selector = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(prev_auton_button_selector, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(prev_auton_button_selector, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(prev_auton_button_selector, LV_BTN_ACTION_CLICK, onPrevPress); 
    lv_obj_align(prev_auton_button_selector, NULL, LV_ALIGN_CENTER, -145, 30);
	lv_obj_set_height(prev_auton_button_selector, 40);
	lv_obj_set_width(prev_auton_button_selector, 130);
    lv_obj_set_style(prev_auton_button_selector, &sensor_button_style);
	lv_obj_t *prev_buttonText_auton = lv_label_create(home_page, NULL);
    lv_label_set_text(prev_buttonText_auton, "");  
    lv_obj_set_x(prev_buttonText_auton, 50); 
    prev_buttonText_auton = lv_label_create(prev_auton_button_selector, NULL);
    lv_label_set_text(prev_buttonText_auton, SYMBOL_PREV " PREV");

	select_auton_button_selector = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(select_auton_button_selector, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(select_auton_button_selector, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(select_auton_button_selector, LV_BTN_ACTION_CLICK, selectAuton); 
    lv_obj_align(select_auton_button_selector, NULL, LV_ALIGN_CENTER, -5, 30);
	lv_obj_set_height(select_auton_button_selector, 40);
	lv_obj_set_width(select_auton_button_selector, 130);
    lv_obj_set_style(select_auton_button_selector, &sensor_button_style);
	lv_obj_t *select_buttonText_auton = lv_label_create(home_page, NULL);
    lv_label_set_text(select_buttonText_auton, "");  
    lv_obj_set_x(select_buttonText_auton, 50); 
    select_buttonText_auton = lv_label_create(select_auton_button_selector, NULL);
    lv_label_set_text(select_buttonText_auton, SYMBOL_UPLOAD " SELECT");

	next_auton_button_selector = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(next_auton_button_selector, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(next_auton_button_selector, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(next_auton_button_selector, LV_BTN_ACTION_CLICK, onNextPress); 
    lv_obj_align(next_auton_button_selector, NULL, LV_ALIGN_CENTER, 135, 30);
	lv_obj_set_height(next_auton_button_selector, 40);
	lv_obj_set_width(next_auton_button_selector, 130);
    lv_obj_set_style(next_auton_button_selector, &sensor_button_style);
	lv_obj_t *next_buttonText_auton = lv_label_create(home_page, NULL);
    lv_label_set_text(next_buttonText_auton, "");  
    lv_obj_set_x(next_buttonText_auton, 50); 
    next_buttonText_auton = lv_label_create(next_auton_button_selector, NULL);
    lv_label_set_text(next_buttonText_auton, "NEXT " SYMBOL_NEXT);

	return_auton_button_selector = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(return_auton_button_selector, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(return_auton_button_selector, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(return_auton_button_selector, LV_BTN_ACTION_CLICK, return_to_home); 
    lv_obj_align(return_auton_button_selector, NULL, LV_ALIGN_CENTER, 100, 110);
	lv_obj_set_height(return_auton_button_selector, 30);
	lv_obj_set_width(return_auton_button_selector, 160);
    lv_obj_set_style(return_auton_button_selector, &sensor_button_style);
	lv_obj_t *return_buttonText_auton = lv_label_create(home_page, NULL);
    lv_label_set_text(return_buttonText_auton, "");  
    lv_obj_set_x(return_buttonText_auton, 50); 
    return_buttonText_auton = lv_label_create(return_auton_button_selector, NULL);
    lv_label_set_text(return_buttonText_auton, SYMBOL_CLOSE" Return");

	// game page
    controller_status_game =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(controller_status_game, "Controller Status: Adequate ");
    lv_obj_align(controller_status_game, NULL, LV_ALIGN_IN_LEFT_MID, 20, -75);

    battery_percent_game =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(battery_percent_game, "Battery Percentage: 100%");
    lv_obj_align(battery_percent_game, NULL, LV_ALIGN_IN_LEFT_MID, 20, -55);

    battery_temp_game =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(battery_temp_game, "Battery Temperature: 100%");
    lv_obj_align(battery_temp_game, NULL, LV_ALIGN_IN_LEFT_MID, 20, -35);

    time_since_startup_game =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(time_since_startup_game, "Time Since Startup: 0ms");
    lv_obj_align(time_since_startup_game, NULL, LV_ALIGN_IN_LEFT_MID, 20, -15);

    competition_stat_game =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(competition_stat_game, "Competition Status: ENABLED");
    lv_obj_align(competition_stat_game, NULL, LV_ALIGN_IN_LEFT_MID, 20, 5);

	return_button_game = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(return_button_game, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(return_button_game, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(return_button_game, LV_BTN_ACTION_CLICK, return_to_home); 
    lv_obj_align(return_button_game, NULL, LV_ALIGN_CENTER, 100, 110);
	lv_obj_set_height(return_button_game, 30);
	lv_obj_set_width(return_button_game, 160);
    lv_obj_set_style(return_button_game, &sensor_button_style);
	lv_obj_t *return_buttonText_game = lv_label_create(home_page, NULL);
    lv_label_set_text(return_buttonText_game, "");  
    lv_obj_set_x(return_buttonText_game, 50); 
    return_buttonText_game = lv_label_create(return_button_game, NULL);
    lv_label_set_text(return_buttonText_game, SYMBOL_CLOSE " Return");

	// misc page
    debug_line1_misc =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(debug_line1_misc, "Debug Line  ");
    lv_obj_align(debug_line1_misc, NULL, LV_ALIGN_IN_LEFT_MID, 20, -75);

    debug_line2_misc =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(debug_line2_misc, "Debug Line  ");
    lv_obj_align(debug_line2_misc, NULL, LV_ALIGN_IN_LEFT_MID, 20, -55);

    debug_line3_misc =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(debug_line3_misc, "Debug Line  ");
    lv_obj_align(debug_line3_misc, NULL, LV_ALIGN_IN_LEFT_MID, 20, -35);

    debug_line4_misc =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(debug_line4_misc, "Debug Line  ");
    lv_obj_align(debug_line4_misc, NULL, LV_ALIGN_IN_LEFT_MID, 20, -15);

    debug_line5_misc =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(debug_line5_misc, "Debug Line  ");
    lv_obj_align(debug_line5_misc, NULL, LV_ALIGN_IN_LEFT_MID, 20, 5);

    debug_line6_misc =  lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(debug_line6_misc, "Debug Line  ");
    lv_obj_align(debug_line6_misc, NULL, LV_ALIGN_IN_LEFT_MID, 20, 25);

	return_button_misc = lv_btn_create(lv_scr_act(), NULL);
	lv_btn_set_style(return_button_misc, LV_BTN_STYLE_REL, &sensor_button_style);
	lv_btn_set_style(return_button_misc, LV_BTN_STYLE_PR, &sensor_button_style);
	lv_btn_set_action(return_button_misc, LV_BTN_ACTION_CLICK, return_to_home); 
    lv_obj_align(return_button_misc, NULL, LV_ALIGN_CENTER, 100, 110);
	lv_obj_set_height(return_button_misc, 30);
	lv_obj_set_width(return_button_misc, 160);
    lv_obj_set_style(return_button_misc, &sensor_button_style);
	lv_obj_t *return_buttonText_misc = lv_label_create(home_page, NULL);
    lv_label_set_text(return_buttonText_misc, "");  
    lv_obj_set_x(return_buttonText_misc, 50); 
    return_buttonText_misc = lv_label_create(return_button_misc, NULL);
    lv_label_set_text(return_buttonText_misc, SYMBOL_CLOSE " Return");

	hide_sensorPage();
	hide_autonPage();
	hide_gamePage();
	hide_miscPage();
 
	//-- Reset sensors and auton selector init //--
	pros::delay(3000);
	data.reset_all_primary_sensors();
	odom.set_horizontal_tracker_specs(2.75, 1.7);
	odom.set_vertical_tracker_specs(2.75, 4.6);
	imu_sensor.tare_rotation();
	odom_piston.set_value(false);
	climber.set_value(false);
	// horizontal_rotation_sensor.set_position(0);
	utility::restart_all_chassis_motors(false);
	sprintf(buffer, SYMBOL_LIST " Selected Path %d: %s", selected_auton, auton_Legend[selected_auton].c_str());
    lv_label_set_text(current_auton_display_selector, buffer);
    intake_motor.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
	flywheel_arm.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
	// select.receive_selector_input(time); // Enabled Auton Selector (STEP 1)
}

//--DONT TOUCH THESE FUNCTIONS--\*
void disabled() {}
void competition_initialize() {}
//------------------------------\*

void init_extend_piston() {
	odom_piston.set_value(false);
	pros::delay(200);
	odom_piston.set_value(true);
	pros::delay(300);
}

void move_arm_down() {
	while (true) {
		//flywheel_arm.move_voltage(-10000); 
		if (cata_sensor.get_value() == 1) {
			//flywheel_arm.move_voltage(0);  // once arm is down, stop motor
            arm_down = false;
            down_flywheel_enabled = true;
            cata_toggled = false;
			break;
		}
	}
}

void moveToUnderGoalNorCal(){
	double end_point_tolerance = 10;
    std::vector<CurvePoint> Path;
	bool reverse = true;

	double end_pose_x = 23; double end_pose_y = -20;

    CurvePoint StartPos(utility::get_x(), utility::get_y(), 4, 2, 20, 5, 1);
    CurvePoint newPoint0(5, 0, 1, 2, 40, 5, 1);
    CurvePoint newPoint1(9, -8, 1, 2, 40, 5, 1);
    CurvePoint newPoint2(12, -17, 1, 2, 40, 5, 1);
    CurvePoint newPoint4(end_pose_x, end_pose_y, 2, 1, 20, 5, 1);
    Path.push_back(StartPos); Path.push_back(newPoint0); Path.push_back(newPoint1); Path.push_back(newPoint2); Path.push_back(newPoint4); 

	bool downOnce = false;

    while (true){ 
		odom.update_odom();
		if(fabs(sqrt(pow(12 - utility::get_x(), 2) + pow(-17 - utility::get_y(), 2))) <= 10 && downOnce == false){
			odom.update_odom();
			left_wing.set_value(true);
			pros::delay(300);
			utility::motor_deactivation();
			left_wing.set_value(false);
			odom.update_odom();
			downOnce = true;
		}
		if(fabs(sqrt(pow(end_pose_x - utility::get_x(), 2) + pow(end_pose_y - utility::get_y(), 2))) <= fabs(end_point_tolerance)){
			mtp.set_mtp_constants(6, 0, 5, 35, 70, 40);
			mtp.move_to_point(end_pose_x, end_pose_y, reverse, false, 0.5);
			utility::motor_deactivation();
			break;
		}
		FollowCurve(Path, 10, 10, 90, reverse);
		pros::delay(10);
	}
}


void full6ballprovs() {

	odom_piston.set_value(false);
	intake_motor.move_voltage(12000);
	pros::delay(200);
	intake_motor.move_voltage(-12000);

	rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(-90, 110);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(31, 90, 5, true);

	moveToUnderGoalNorCal();

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-160, 110);

    mov_t.set_t_constants(12, 0, 35, 500);
	mov_t.set_translation_pid(-15, 127, 1, false);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(8, 127, 1, false);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-90, 110);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-5, 110);

	intake_motor.move_voltage(12000);

	mov_t.set_t_constants(12, 0, 35, 500);
	mov_t.set_translation_pid(14, 127, 1, false);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-13, 110, 2, false);

 	rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-55, 110);

	intake_motor.move_voltage(-12000);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(34, 110, 4, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-90, 110);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(14, 90, 1, true);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-6, 110, 2, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(60, 90);

	intake_motor.move_voltage(12000);
	pros::delay(500);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(-46, 90);

	intake_motor.move_voltage(-12000);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(27, 110, 3, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(90, 90);

	intake_motor.move_voltage(12000);

	left_front_wing.set_value(true);
	right_front_wing.set_value(true);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(35, 110, 8, false);

	left_front_wing.set_value(false);
	right_front_wing.set_value(false);

    cur_c.set_c_constants(6, 0, 45);
    cur_c.set_curve_pid(0, 100, 0.2, 4, true);

}

void closeSideProvs(){
	left_front_wing.set_value(true);
	intake_motor.move_voltage(12000);
	pros::delay(300);
	intake_motor.move_voltage(-12000);
	left_front_wing.set_value(false);

	mtp.set_mtp_constants(7, 0, 6, 35, 110, 110);
	mtp.move_to_point(48, -14, false, false, 3);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(10, 0, true, false, 3);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(160, 90);

	intake_motor.move_voltage(12000);
	pros::delay(700);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-90, 90);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(11, 70, 3, true);

    cur_c.set_c_constants(6, 0, 45); 
    cur_c.set_curve_pid(-45, 100, 0.2, 4, false);

	left_wing.set_value(true);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-18, 40, 1.2, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-90, 90);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-45, 90);
	
	mov_t.set_t_constants(5, 0, 35, 500); 
	mov_t.set_translation_pid(3, 70, 1, true); 

	left_wing.set_value(false);

    cur_c.set_c_constants(6, 0, 45);
    cur_c.set_curve_pid(-90, 90, 0.31, 1.5, true); 

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(0, 60);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(88, 60);

	intake_motor.move_voltage(12000);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(29, 70, 3, true); 

	right_front_wing.set_value(false);
}

void prototyperush6() {
	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(53, 14, false, false, 3);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(6, 3, true, false, 3);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(45, 90);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-88, 90);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(5, 30, false, false, 3);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(10, -14, true, false, 3);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(25, -28, true, false, 3);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-180, 90);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-15, 90, 1.5, true);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(8, 90, 2, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-10, 110);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(10, 90, 0.8, true);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-6, 90, 2, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-90, 90);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(67, 37, false, false, 1);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-90, 90);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-13, 110, 2, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(60, 90);

	intake_motor.move_voltage(12000);
	pros::delay(500);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(0, 90);

	intake_motor.move_voltage(-12000);


	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(84, 15, false, false, 1.8);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(78, -38, false, false, 1.5);

    cur_c.set_c_constants(6, 0, 45);
    cur_c.set_curve_pid(0, 90, 0.3, 3, true);
}


void moveToUnderGoalProvsSkills(){
	double end_point_tolerance = 10;
    std::vector<CurvePoint> Path;
	bool reverse = false;

	double end_pose_x = 40; double end_pose_y = 20;

    CurvePoint StartPos(utility::get_x(), utility::get_y(), 4, 2, 20, 5, 1);
    CurvePoint newPoint0(9, 13, 1, 2, 40, 5, 1);
    CurvePoint newPoint1(20, 20, 1, 2, 40, 5, 1);
    CurvePoint newPoint4(end_pose_x, end_pose_y, 2, 1, 20, 5, 1);
    Path.push_back(StartPos); Path.push_back(newPoint0); Path.push_back(newPoint4); 

	bool downOnce = false;

    while (true){ 
		odom.update_odom();
		if(fabs(sqrt(pow(end_pose_x - utility::get_x(), 2) + pow(end_pose_y - utility::get_y(), 2))) <= fabs(end_point_tolerance)){
			mtp.set_mtp_constants(8, 0, 6, 35, 120, 90);
			mtp.move_to_point(end_pose_x, end_pose_y, reverse, false, 2);
			utility::motor_deactivation();
			break;
		}
		FollowCurve(Path, 7, 12, 127, reverse); // circle size, linear kp, linear max speed
		pros::delay(10);
	}
}

void ProvsSkills() {


    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-45, 110);

	intake_motor.move_voltage(12000);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(32, 17, false, false, 1.8);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(17, 16, true, false, 1.8);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(70, 90);

    mov_t.set_t_constants(8, 0, 35, 500);
	mov_t.set_translation_pid(-4, 50, 2, true);

	right_wing.set_value(true);
    cata_motor.move_voltage(8750);
    flywheel_arm.move_voltage(-8750);
	pros::delay(21000);
    cata_motor.move_voltage(0);
    flywheel_arm.move_voltage(0);
	right_wing.set_value(false);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(26, -20, false, false, 3);

	right_front_wing.set_value(true);

	mtp.set_mtp_constants(9, 0, 5, 35, 110, 110);
	mtp.move_to_point(80, -29, false, false, 2);

	right_front_wing.set_value(false);

	mtp.set_mtp_constants(9, 0, 5, 35, 110, 110);
	mtp.move_to_point(110, 8, true, false, 3);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(45, 90);

	mtp.set_mtp_constants(9, 0, 5, 35, 110, 110);
	mtp.move_to_point(117, -10, false, false, 2);

	mtp.set_mtp_constants(14, 0, 5, 35, 110, 110);
	mtp.move_to_point(120, -80, false, false, 5);

	right_front_wing.set_value(true);

	mtp.set_mtp_constants(9, 0, 5, 35, 110, 110);
	mtp.move_to_point(100, -105, false, false, 2);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(180, 90);

    mov_t.set_t_constants(9, 0, 35, 500);
	mov_t.set_translation_pid(18, 110, 1, true);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-15, 110, 1.2, true);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(180, 90);

    mov_t.set_t_constants(9, 0, 35, 500);
	mov_t.set_translation_pid(16, 110, 1.8, true);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-17, 110, 1.5, true);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(180, 90);

	right_front_wing.set_value(false);

	mtp.set_mtp_constants(9, 0, 7, 35, 110, 110);
	mtp.move_to_point(75, -70, true, false, 3);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(-90, 90);

	left_wing.set_value(true);
	right_wing.set_value(true);

	mtp.set_mtp_constants(16, 0, 5, 35, 110, 110); // hit 1
	mtp.move_to_point(40, -120, true, false, 2);

	left_wing.set_value(false);
	right_wing.set_value(false);

	mtp.set_mtp_constants(9, 0, 5, 35, 110, 110);
	mtp.move_to_point(80, -70, false, false, 2);

	mtp.set_mtp_constants(9, 0, 5, 35, 110, 110);
	mtp.move_to_point(50, -70, false, false, 2);

	left_wing.set_value(true);
	right_wing.set_value(true);

	mtp.set_mtp_constants(16, 0, 5, 35, 110, 110); // hit 2
	mtp.move_to_point(50, -120, true, false, 2);

	mtp.set_mtp_constants(9, 0, 5, 35, 110, 110);
	mtp.move_to_point(30, -70, false, false, 1.5);

	left_wing.set_value(false);
	right_wing.set_value(false);

	mtp.set_mtp_constants(16, 0, 5, 35, 110, 110); // hit 3
	mtp.move_to_point(50, -120, true, false, 1.5);

	mtp.set_mtp_constants(9, 0, 5, 35, 110, 110);
	mtp.move_to_point(10, -70, true, false, 1.5);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(90, 90);

    mov_t.set_t_constants(9, 0, 35, 500);
	mov_t.set_translation_pid(19, 110, 1, true);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(160, 90);

    mov_t.set_t_constants(9, 0, 35, 500);
	mov_t.set_translation_pid(40, 110, 1.5, true);

    mov_t.set_t_constants(9, 0, 35, 500);
	mov_t.set_translation_pid(-5, 110, 3, true);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(90, 90);

    cur_c.set_c_constants(6, 0, 45);
    cur_c.set_curve_pid(0, 90, 0.25, 3, false);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(0, 90);

    mov_t.set_t_constants(9, 0, 35, 500);
	mov_t.set_translation_pid(18, 110, 1, true);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-10, 110, 1.2, true);

    mov_t.set_t_constants(9, 0, 35, 500);
	mov_t.set_translation_pid(16, 110, 1, true);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-12, 110, 1.5, true);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(-135, 90);

    cur_c.set_c_constants(6, 0, 45);
    cur_c.set_curve_pid(-90, 90, 0.55, 3, false);

	primary_climber.set_value(true);

    mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(56, 110, 1.2, true);

	primary_climber.set_value(false);
	odom_piston.set_value(true); 
}

void closeSideNorCal(){
	left_front_wing.set_value(true);
	intake_motor.move_voltage(12000);
	pros::delay(300);
	intake_motor.move_voltage(-12000);
	left_front_wing.set_value(false);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(16.5, 90);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(49, 90, 3, true);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-42, 90, 3, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(160, 90);

	intake_motor.move_voltage(12000);
	pros::delay(700);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-90, 90);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(11, 70, 3, true);

    cur_c.set_c_constants(6, 0, 45); 
    cur_c.set_curve_pid(-45, 100, 0.2, 4, false);

	left_wing.set_value(true);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-18, 40, 2, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-90, 90);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-45, 90);
	
	mov_t.set_t_constants(5, 0, 35, 500); 
	mov_t.set_translation_pid(3, 70, 1, true); 

	left_wing.set_value(false);

    cur_c.set_c_constants(6, 0, 45);
    cur_c.set_curve_pid(-90, 90, 0.34, 2, true);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(0, 60);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(88, 60);

	intake_motor.move_voltage(12000);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(20, 70, 3, true); 
}

void provsElimRush() {
	
	left_front_wing.set_value(true);
	intake_motor.move_voltage(12000);
	pros::delay(100);
	intake_motor.move_voltage(-12000);
	left_front_wing.set_value(false);

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(16.5, 90);

	mov_t.set_t_constants(12, 0, 35, 500);
	mov_t.set_translation_pid(38, 120, 3, true);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(90, 90);

	left_front_wing.set_value(true);
	intake_motor.move_voltage(12000);
	left_wing.set_value(true);

	mov_t.set_t_constants(12, 0, 35, 500);
	mov_t.set_translation_pid(21, 120, 3, true);

	mov_t.set_t_constants(5, 0, 35, 500);
	mov_t.set_translation_pid(-10, 90, 3, true);
}

/**
 * @brief Main autonomous function. PID prereqs:
 * @brief 90 DEGREES CONSTANTS: 6, 0, 45
 * @brief 45 DEGREE CONSTANTS: 6, 0.003, 35
 * @brief PID units are in inches
 *  
 */

void autonomous(){  // Autonomous function control
	global_robot_x = 0;
	global_robot_y = 0;
	utility::set_chassis_to_brake();
	odom_piston.set_value(false);
	slew.set_slew_distance({4, 4});
	slew.set_slew_min_power({40, 40});
	mov_t.set_dt_constants(3.125, 1.6, 600); // Parameters are : Wheel diameter, gear ratio, motor cartridge type
	utility::restart_all_chassis_motors(false);
	// init_extend_piston();
	// move_arm_down();
	// selector.recieve_selector_input(time); // Enabled Auton Selector (STEP 1) ONLY FOR PROTOTYPE USE
	// select.select_current_auton(); // Enable Auton Selector (STEP 2) 

	// closeSideNorCal();
	// new6ballNorCal();
	// NorCalSkills();

	// mov_t.set_t_constants(5, 0, 35, 500);
	// mov_t.set_translation_pid(23, 127, 3, true);

    // cur_c.set_c_constants(6, 0, 45);
    // cur_c.set_curve_pid(90, 90, 0.3, 0.5, false);

    // cur_c.set_c_constants(6, 0, 45);
    // cur_c.set_curve_pid(0, 90, 0.3, 3, false);

    // cur_c.set_c_constants(6, 0, 45);
    // cur_c.set_curve_pid(135, 90, 0.1, 0.6, true);

    // cur_c.set_c_constants(6, 0, 45);
    // cur_c.set_curve_pid(180, 90, 0.1, 0.3, false);

    // cur_c.set_c_constants(6, 0, 45);
    // cur_c.set_curve_pid(-90, 90, 0.1, 3, false);

	// mtp.set_mtp_constants(6, 0, 5, 35, 90, 110);
	// mtp.move_to_point(20, -20, false, false, 3);

	// mtp.set_mtp_constants(6, 0, 5, 35, 90, 110);
	// mtp.move_to_point(40, 20, false, false, 3);

	// mtp.set_mtp_constants(6, 0, 5, 35, 90, 110);
	// mtp.move_to_point(20, 0, false, false, 3);

	// mtp.set_mtp_constants(6, 0, 5, 35, 90, 110);
	// mtp.move_to_point(-20, -20, true, false, 3);

	// mtp.set_mtp_constants(6, 0, 5, 35, 90, 110);
	// mtp.move_to_point(-25, 0, false, false, 1);

	// mtp.set_mtp_constants(6, 0, 5, 35, 90, 110);
	// mtp.move_to_point(-23, 80, false, false, 3);

	// fixingPP();

	// ProvsSkills();
	// full6ballprovs();
	// closeSideProvs();
	// closeSideNorCal();

	provsElimRush();


}

/**
 * @brief Main driver control function. All code used during game phase outside of auton
 * 
 */

void initial_skills_phase() {

    rot_r.set_r_constants(6, 0, 45);
    rot_r.set_rotation_pid(-45, 110);

	intake_motor.move_voltage(12000);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(32, 17, false, false, 1.8);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(17, 16, true, false, 1.8);

    rot_r.set_r_constants(7, 0, 85);
    rot_r.set_rotation_pid(70, 90);

	mtp.set_mtp_constants(7, 0, 5, 35, 110, 110);
	mtp.move_to_point(17, 20, true, false, 0.7);

}

void DRIVER_PHASE() {
	// op_mov.dt_Control();
	op_mov.exponential_curve_accelerator();
	odom.update_odom();
	power_intake();
	extend_wings();
	extend_front_wings();
	extend_odom_piston();
	extend_climber();
	extend_primary_climber();
	realCataControl();
	pros::delay(delayAmount); // Dont hog CPU ;)
}

void CONSTANT_TUNER_PHASE() {
	tuner.driver_tuner();
	pros::delay(10);
}

bool DRIVER_ENABLED = true;

void opcontrol(){ // Driver control function	
	odom_piston.set_value(true); 
	while (true){
		if (DRIVER_ENABLED) { DRIVER_PHASE(); }
		else { CONSTANT_TUNER_PHASE(); }
	}
}

