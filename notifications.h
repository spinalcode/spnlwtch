struct notifications{
	char src[64];
    unsigned long int id;
	char title[64];
	char body[128];
	char sender[64];
};

notifications notification[16];
int numNotifications = 0;

