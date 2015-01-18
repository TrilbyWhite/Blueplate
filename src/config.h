
/* DESKTOP */

static const unsigned long background = 0x202428;
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

/* MAIL */

static MBox box[] = {
	/* path to maildir,                    color,    0 */
	{"/home/jmcclure/mail/umass/INBOX/new", 0x94BBD1, 0},
	{"/home/jmcclure/mail/mccluresk9/INBOX/new", 0x9498A2, 0},
	/* keep a null terminator */
	{NULL, 0, 0},
};

static const XPoint mail_icon_size = { 16, 16 };
static const char mail_icon_data[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x7F,
	0x06, 0x60, 0x0A, 0x50, 0x12, 0x48, 0x22, 0x44,
	0x42, 0x42, 0xA2, 0x45, 0x12, 0x48, 0x0A, 0x50,
	0x06, 0x60, 0xFE, 0x7F, 0x00, 0x00, 0x00, 0x00,
};

