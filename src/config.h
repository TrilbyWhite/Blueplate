
static const unsigned long background = 0x202428;

#ifdef module_desktop
static const unsigned long space_norm = 0x384246;
static const unsigned long space_used = 0x9498A2;
static const unsigned long space_curr = 0x94BBD1;
static Desktop desk[] = {
	/*0,  x, y, w, h */
	{ 0,	2,	2,	5,	5 },
	{ 0,	9,	2,	5,	5 },
	{ 0,	2,	9,	5,	5 },
	{ 0,	9,	9,	5,	5 },
};
#endif /* module_desktop */


#ifdef module_mail
static MBox box[] = {
	/* path to maildir,                    color,    0 */
	{"/home/jmcclure/mail/umass/INBOX/new", 0x94BBD1, 0},
	{"/home/jmcclure/mail/mccluresk9/INBOX/new", 0x9498A2, 0},
	/* keep a null terminator */
	{NULL, 0, 0},
};
// 16x16 outline icon
static const XPoint mail_icon_size = { 16, 16 };
static const char mail_icon_data[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x7F,
	0x06, 0x60, 0x0A, 0x50, 0x12, 0x48, 0x22, 0x44,
	0x42, 0x42, 0xA2, 0x45, 0x12, 0x48, 0x0A, 0x50,
	0x06, 0x60, 0xFE, 0x7F, 0x00, 0x00, 0x00, 0x00,
};

// 13x13 outline icon
//static const XPoint mail_icon_size = { 13, 13 };
//static const char mail_icon_data[] = {
  //0x00, 0x00, 0xFF, 0x1F, 0x03, 0x18, 0x05, 0x14, 0x09, 0x12, 0x11, 0x11, 
  //0xE1, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0xFF, 0x1F, 
  //0x00, 0x00, };
  
// 13x13 filled (shaded) icon
//static const XPoint mail_icon_size = { 13, 13 };
//static const char mail_icon_data[] = {  
  //0x00, 0x00, 0x00, 0x00, 0xFC, 0x07, 0xFA, 0x0B, 0xF6, 0x0D, 0xEE, 0x0E, 
  //0x1E, 0x0F, 0xFE, 0x0F, 0xFE, 0x0F, 0xFE, 0x0F, 0xFE, 0x0F, 0x00, 0x00, 
  //0x00, 0x00, };   
#endif /* module_mail */

#ifdef module_connman
static const short bar_width = 3;		// width of the rectangle (px)
static const short bar_gap = 2;			// gap between rectangles (px)
static const short bar_height = 13;	// height of the rectangles (px)

static const unsigned long conn_undefined		= 0x94BBD1;
static const unsigned long conn_offline		= 0xE81414;
static const unsigned long conn_idle			= 0xE0E00E;
static const unsigned long conn_ready			= 0x19E2DD;
static const unsigned long conn_online			= 0x11EE11;
//static const char *connman_click[] = { NULL };
static const char *connman_click[] = { "cmst", "--disable-tray-icon", NULL };
#endif /* module_connman */
 
#ifdef module_battery
static Battery bat[] = {
	// one line for each battery icon you wish to define.  There can be more
	// icons defined than there are batteries.  Each icon can have its own 
	// color scheme if desired.
	/* outline_color, fill_color, 0, 0, 0 */
	{0xDDDDDD,  0x94BBD1, 0, 0, 0},
	{0xDDDDDD,  0x94BBD1, 0, 0, 0},
	{0xDDDDDD,  0x94BBD1, 0, 0, 0},
};

// Battery fill area, start counting pixels from upper left as 0,0
// Format is x, y, width, height.  Use this to define the hollow area
// of the icon below.
static const XRectangle battery_fill_rect = {3, 2, 7, 10};

// Notification Trigger.  Use a negative number for no notifications,
// or an integer between 0 and 100 representing the percent of charge to
// trigger notifications. This variable may be supplied even if there is
// no notification server running on your machine.
static short notification_trigger = 20;

// 13x13 battery (hollow) icon
static const XPoint battery_icon_size = { 13, 13 };
static const char battery_icon_data[] = {
  0xE0, 0x00, 0xF8, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
  0xF8, 0x03, };

// 13x13 plug charging icon
static const XRectangle battery_charging_size = {13, 13 };
static const char battery_charging_data[] = {
  0x08, 0x02, 0x08, 0x02, 0x08, 0x02, 0x08, 0x02, 0xFC, 0x07, 0x02, 0x08, 
  0x02, 0x08, 0x02, 0x08, 0x02, 0x08, 0x04, 0x04, 0x18, 0x03, 0xE0, 0x00, 
  0x40, 0x00, };  
  
// 13x13 plug full icon
static const XRectangle battery_full_size = {13, 13 };
static const char battery_full_data[] = {
  0x08, 0x02, 0x08, 0x02, 0x08, 0x02, 0x08, 0x02, 0xFC, 0x07, 0xFE, 0x0F, 
  0xFE, 0x0F, 0xFE, 0x0F, 0xFE, 0x0F, 0xFC, 0x07, 0xF8, 0x03, 0xE0, 0x00, 
  0x40, 0x00, };
#endif /* module_battery */  	
