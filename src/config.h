
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
	{"/home/USER/mail/ACCOUNT1/INBOX/new", 0x94BBD1, 0},
	{"/home/USER/mail/ACCOUNT2/INBOX/new", 0x9498A2, 0},
	/* keep a null terminator */
	{NULL, 0, 0},
};

static const XPoint mail_icon_size = { 16, 16 };
static const char mail_icon_data[] = {
	0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0x06, 0xC0,
	0x0A, 0xA0, 0x12, 0x90, 0x22, 0x88, 0x42, 0x84,
	0x82, 0x82, 0x22, 0x89, 0x12, 0x90, 0x0A, 0xA0,
	0x06, 0xC0, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00,
};

