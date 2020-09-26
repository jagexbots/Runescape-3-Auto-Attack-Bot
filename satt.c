/*
--------------------------------------------------
    Andrew Gower 2020 - A Runescape 3 Bot
--------------------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>
#include <pthread.h>

#pragma GCC diagnostic ignored "-Wunused-result"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define uint unsigned int
#define click_delay 1 //qRand(2, 3)

Display *d;
int si;
XEvent event;

Window twin;

unsigned char rd = 75;
unsigned char gd = 79;
unsigned char bd = 137;
unsigned char tol = 6;
uint roam = 0;
uint degrade = 0;
uint debug = 1;
float sm = 1;
uint silent_hunt = 0;
int bones = 0;

uint enable = 0;
uint hunt = 0;
uint delaymicro = 10000;
time_t nt = 0;
unsigned int lx=0, ly=0, ox=0, oy=0;


/***************************************************
   ~~ Utils
*/
uint qRand(const uint min, const uint umax)
{
    static time_t ls = 0;
    if(time(0) > ls)
    {
        srand(time(0));
        ls = time(0) + 33;
    }
    const int rv = rand();
    const uint max = umax + 1;
    if(rv == 0)
        return min;
    return ( ((float)rv / RAND_MAX) * (max-min) ) + min; //(rand()%(max-min))+min;
}

float qRandFloat(const float min, const float max)
{
   static time_t ls = 0;
    if(time(0) > ls)
    {
        srand(time(0));
        ls = time(0) + 33;
    }
    const float rv = (float)rand();
    const float rmax = (float)RAND_MAX;
    if(rv == 0)
        return min;
    return ( (rv / rmax) * (max-min) ) + min;
}

//https://www.cl.cam.ac.uk/~mgk25/ucs/keysymdef.h
int key_is_pressed(KeySym ks)
{
    Display *dpy = XOpenDisplay(":0");
    char keys_return[32];
    XQueryKeymap(dpy, keys_return);
    KeyCode kc2 = XKeysymToKeycode(dpy, ks);
    int isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
    XCloseDisplay(dpy);
    return isPressed;
}

int isFocus()
{
    Display *d;
    int si;
    XEvent event;

    //Open Display 0
    d = XOpenDisplay((char *) NULL);
    if(d == NULL)
        return 0;

    //Get default screen
    si = XDefaultScreen(d);

    //Reset mouse event
    memset(&event, 0x00, sizeof(event));

    //Find target window
    XQueryPointer(d, RootWindow(d, si), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    event.xbutton.subwindow = event.xbutton.window;
    while(event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(d, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    }

    // out of bonds?
    if(event.xbutton.window != twin)
    {
        XCloseDisplay(d);
        return 0;
    }

    XCloseDisplay(d);
    return 1;
}

void customSet()
{
    Display *d;
    int si;
    XEvent event;

    //Open Display 0
    d = XOpenDisplay((char *) NULL);
    if(d == NULL)
        return;

    //Get default screen
    si = XDefaultScreen(d);

    //Reset mouse event
    memset(&event, 0x00, sizeof(event));

    //Find target window
    XQueryPointer(d, RootWindow(d, si), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    event.xbutton.subwindow = event.xbutton.window;
    while(event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(d, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    }

    // out of bonds?
    if(event.xbutton.window != twin)
    {
        XCloseDisplay(d);
        return;
    }
    XWindowAttributes attr;
    XGetWindowAttributes(d, event.xbutton.window, &attr);
    if(event.xbutton.x >= attr.width-1 || event.xbutton.y >= attr.height-1 || event.xbutton.x <= 1 || event.xbutton.y <= 1)
    {
        XCloseDisplay(d);
        return;
    }
    // grab scan rect
    XImage *img = XGetImage(d, event.xbutton.window, event.xbutton.x, event.xbutton.y, 1, 1, AllPlanes, XYPixmap);
    if(img != NULL)
    {
        XColor c;
        c.pixel = XGetPixel(img, 0, 0);
        bd = c.pixel & img->blue_mask;
        gd = (c.pixel & img->green_mask) >> 8;
        rd = (c.pixel & img->red_mask) >> 16;
        debug = 0;
        degrade = 1;
        tol = 6;
        printf("SET: %u %u %u\n", rd, gd, bd);
        usleep(300000);
        XFree(img);
    }
    XCloseDisplay(d);
}

int isBlack()
{
    // scan rect
    const uint cmx = 6, cmy = 6;

    //Find target window
    XQueryPointer(d, RootWindow(d, si), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    event.xbutton.subwindow = event.xbutton.window;
    while(event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(d, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    }

    // out of bonds?
    if(event.xbutton.window != twin)
        return 0;
    XWindowAttributes attr;
    XGetWindowAttributes(d, event.xbutton.window, &attr);
    if(event.xbutton.x >= attr.width-cmx-1 || event.xbutton.y >= attr.height-cmy-1 || event.xbutton.x <= cmx+1 || event.xbutton.y <= cmy+1)
        return 0;
    
    // grab scan rect
    XImage *img = XGetImage(d, event.xbutton.window, event.xbutton.x-(cmx/2), event.xbutton.y-(cmy/2), cmx, cmy, AllPlanes, XYPixmap);
    if(img != NULL)
    {
        XColor c;
        uint bp = 0;
        for(int y = 0; y < cmy; y++)
        {
            for(int x = 0; x < cmx; x++)
            {
                c.pixel = XGetPixel(img, x, y);
                const unsigned char b = c.pixel & img->blue_mask;
                const unsigned char g = (c.pixel & img->green_mask) >> 8;
                const unsigned char r = (c.pixel & img->red_mask) >> 16;
                if(r == 0 && g == 0 && b == 0)
                    bp++;
            }
        }
        XFree(img);
        if(bp == cmx*cmy)
            return 1;
    }
    return 0;
}

int isEnemy(const unsigned char tol, const uint sx, const uint sy)
{
    // scan rect
    const uint cmx = 4, cmy = 4;

    // out of bonds?
    if(event.xbutton.window != twin)
        return 0;
    XWindowAttributes attr;
    XGetWindowAttributes(d, event.xbutton.window, &attr);
    if(sx >= attr.width-cmx-1 || sy >= attr.height-cmy-1 || sx <= cmx+1 || sy <= cmy+1)
        return 0;
    
    // grab scan rect
    XImage *img = XGetImage(d, event.xbutton.window, sx-(cmx/2), sy-(cmy/2), cmx, cmy, AllPlanes, XYPixmap);
    if(img != NULL)
    {
        XColor c;
        for(int y = 0; y < cmy; y++)
        {
            for(int x = 0; x < cmx; x++)
            {
                c.pixel = XGetPixel(img, x, y);
                const unsigned char b = c.pixel & img->blue_mask;
                const unsigned char g = (c.pixel & img->green_mask) >> 8;
                const unsigned char r = (c.pixel & img->red_mask) >> 16;
                if(r == 0 && g == 0 && b == 0)
                {
                    XFree(img);
                    return 0; //Any black pixels abort !!
                }
                if(r > rd-tol && r < rd+tol &&
                   g > gd-tol && g < gd+tol &&
                   b > bd-tol && b < bd+tol )
                {
                    XFree(img);
                    return 1; // one of the pixels was in detectable range
                }
                //is bones?
                if(bones == 1)
                {
                    if(r > 205-6 && r < 205+6 &&
                       g > 202-6 && g < 202+6 &&
                       b > 209-6 && b < 209+6 )
                    {
                        XFree(img);
                        return 1; // oftpidr
                    }
                }
                else if(bones == 2)
                {
                    if(r > 102-3 && r < 102+3 &&
                       g > 157-3 && g < 157+3 &&
                       b > 197-3 && b < 197+3 )
                    {
                        XFree(img);
                        return 1; // oftpidr
                    }
                }
            }
        }
        XFree(img);
    }
    return 0;
}

// simple visual que signal
void sendSignal(const uint n)
{
    Display *d;
    int si;
    XEvent event;

    //Open Display 0
    d = XOpenDisplay((char *) NULL);
    if(d == NULL)
        return;

    //Get default screen
    si = XDefaultScreen(d);

    //Reset mouse event
    memset(&event, 0x00, sizeof(event));

    //Ready to press down mouse 1
    event.type = ButtonPress;
    event.xbutton.button = Button1;
    event.xbutton.same_screen = True;
    
    //Find target window
    XQueryPointer(d, RootWindow(d, si), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    event.xbutton.subwindow = event.xbutton.window;
    while(event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(d, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    }

    for(uint i = 0; i < n; i++)
    {
        //Ready to press down mouse 1
        event.type = ButtonPress;
        event.xbutton.button = Button1;
        event.xbutton.same_screen = True;

        //Fire mouse down
        XSendEvent(d, PointerWindow, True, 0xfff, &event);
        XFlush(d);
        
        //Wait 100ms
        usleep(100000);
        
        //Release mouse down
        event.type = ButtonRelease;
        event.xbutton.state = 0x100;
        XSendEvent(d, PointerWindow, True, 0xfff, &event);
        XFlush(d);
        
        usleep(100000);
    }

    XCloseDisplay(d);
}

/***************************************************
   ~~ Input Thread
*/
void *inputThread(void *arg)
{
    while(1)
    {
        // 10ms
        usleep(10000);

        //Inputs / Keypress
        if(key_is_pressed(XK_Control_L) && key_is_pressed(XK_Alt_L))
        {
            if(enable == 0)
            {
                //Find target window
                d = XOpenDisplay((char *) NULL);
                if(d == NULL)
                    continue;
                si = XDefaultScreen(d);
                XQueryPointer(d, RootWindow(d, si), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
                event.xbutton.subwindow = event.xbutton.window;
                while(event.xbutton.subwindow)
                {
                    event.xbutton.window = event.xbutton.subwindow;
                    XQueryPointer(d, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
                }
                twin = event.xbutton.window;
                XCloseDisplay(d);

                enable = 1;
                if(hunt == 1)
                    delaymicro = 100;
                else
                    delaymicro = 10000;
                usleep(300000);
                printf("AUTO-CLICK: ON\n");
                if(hunt == 0)
                    sendSignal(2);
            }
            else
            {
                enable = 0;
                lx = 0, ly = 0;
                printf("AUTO-CLICK: OFF\n");
                usleep(300000);
            }
        }

        if(enable > 0 && key_is_pressed(XK_X) && isFocus() == 1)
        {
            if(enable == 1)
            {
                enable = 2;
                nt = time(0);
                printf("SILENT-CLICK: ON\n");
                sendSignal(2);
            }
            else
            {
                enable = 1;
                nt = time(0);
                lx = 0, ly = 0;
                printf("SILENT-CLICK: OFF\n");
                sendSignal(2);
            }
        }

        if(enable == 1 && key_is_pressed(XK_C) && isFocus() == 1)
        {
            if(hunt == 0)
            {
                hunt = 1;
                nt = time(0);
                delaymicro = 100;
                printf("AUTO-HUNT: ON\n");
                usleep(300000);
            }
            else
            {
                hunt = 0;
                nt = time(0);
                delaymicro = 10000;
                printf("AUTO-HUNT: OFF\n");
                usleep(300000);
            }
        }

        if(enable == 1 && key_is_pressed(XK_V) && isFocus() == 1)
            customSet();
    }
}

/***************************************************
   ~~ Program Entry Point
*/
int main(int argc, char *argv[])
{
    if(argc >= 2)
    {
        if(strcmp(argv[1], "install") == 0)
        {
            system("wget -O - https://content.runescape.com/downloads/ubuntu/runescape.gpg.key | apt-key add -");
            system("mkdir -p /etc/apt/sources.list.d");
            system("echo \"deb https://content.runescape.com/downloads/ubuntu trusty non-free\" > /etc/apt/sources.list.d/runescape.list");
            system("apt-get update");
            system("apt-get install -y runescape-launcher");
            return 0;
        }

        if(strcmp(argv[1], "help") == 0)
        {
            printf("\n");
            printf("This is a bot for Runescape.com / RS3.\n");
            printf("\n");
            printf("There is the default entity auto click option, which is the base functionality, when the mouse is hovered over an entity in game it will automatically click it. This is handy with flinging fish for example. Or for automatic teleport or alchemisation leveling, if your alchemy tab and inventory tab areon the same window you can put your noted stack of items intended for alchemisation in a grid space that is at the same position as the high level alchemy button, then the entity auto-click will iterate though the entire noted item without needing to move the mouse.\n");
            printf("\n");
            printf("Then there is the hotspot silent click, this is handy for mining stamina if not using perfect juju in combination with stone spirit, also for example, at the wilderness necrite spot, if alligned correctly it can mine and auto kill any goblins that approach you and get between your cursor and the mining rock. \n");
            printf("\n");
            printf("With some intuition these two basic functions can get you a long way in many skills, for example, I have once successfully theived the seeds stall in draynor for many hours.\n");
            printf("\n");
            printf("~~~ Finally is the auto-hunt ~~~\n");
            printf("First rule of thumb using the attack bot is to set the view to top-down birdseye view with preferably maximum outward zoom so that more ground space is visible, there is a setting to increase the zoom distance in the RS3 settings. Also, it does not really matter what graphics settings you have, on my desktop I have textures enabled with no shadows for performance and on my VPS I have everything on low.\n");
            printf("~\n");
            printf("Random Roam - If no enemy was detected at the end of a scan sweep, a random click will be placed on the circumference of the scan max_radius.\n");
            printf("~\n");
            printf("Collect Drops - This will detect bones on the ground and click on them, two settings one for white bones (1) and two for slightly blue bones such as in taverley (2).\n");
            printf("~\n");
            printf("Silent Hunt - This uses silent mouse clicks where applicable, so that you can still interact with the Runescape UI's while the bot is running. May be a detection risk if used for hours at a time?\n");
            printf("~\n");
            printf("Scan Speed Multiplier - On some systems with lower end graphics units the base scan speed may seem a little slow, so bump it up with this floating-point multiplier.\n");
            printf("~\n");
            printf("Start Scan Radius - Scanning for enemies starts at the center of the screen and sweeps outward, this defines the starting radius of the sweep from the center of the screen.\n");
            printf("~\n");
            printf("Max Scan Radius - Scanning for enemies starts at the center of the screen and sweeps outward, this defines the max radius of the sweep from the center of the screen.\n");
            printf("~~~\n");
            printf("\n");
            printf("This is a fairly simple, but very effective bot, and at the time of writing this I have never received a ban from using it - despite aggressively botting my combat for days at a time non-stop albeit on quiet servers.\n");
            printf("\n");
            printf("I've never officialy released this and well - never indend to; so if you have obtained a copy of this, please do not re-distribute or you are risking Jagex updating their heuristics to detect this bot and as such your accounts being banned in the future.\n");
            printf("\n");
            printf("Install Runescape:\n");
            printf("./satt install\n");
            printf("\n");
            printf("~ Andrew\n");
            printf("\n");
            return 0;
        }
    }

    printf("\n\e[1;33;49mAndrew C Gower\e[0m \e[3;35;49m(a.c.gower@gmail.com)\e[0m\n\e[3mI guess I call this the RS3 Auto/Silent Click Attack Bot (ASCAB).\e[0m\n\n");
    printf("./satt <attack type> <random roam 0/1> <collect drops 0/2> <silent hunt 0/1> <scan speed multiplier> <start scan radius in pixels> <max scan radius in pixels>\n\n");
    printf("\033[1mMore information:\033[0m\n");
    printf("./satt help\n\n");
    printf("\033[1mAttack Types:\033[0m \e[033;33m(first three are types of blue dragon)\e[0m\n");
    printf("hero, baby, taverley(2), green, red(2), black, iron(2), steel, mithril, any\n\n");
    printf("\033[1mL-CTRL + L-ALT\033[0m = Auto Click ON/OFF\n\n");
    printf("\033[1mOnce Auto Click is Enabled:\033[0m\n");
    printf("Z = Disable all while pressed\n");
    printf("X = Fixed Silent Click ON/OFF\n");
    printf("C = Auto Hunt ON/OFF\n");
    printf("V = Custom Attack Type [sample at cursor pos]\n\n");

    // default attack anything that lights up with a red hue
    rd = 80;
    gd = 18;
    bd = 27;
    tol = 9;

    // enemy type for auto hunt (mithril dragon is default)
    if(argc >= 2)
    {
        degrade = 1;

        if(strcmp(argv[1], "any") == 0) // Hero's guild blue dragon
        {
            rd = 80;
            gd = 18;
            bd = 27;
            tol = 9;
            degrade = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "hero") == 0) // Hero's guild blue dragon
        {
            rd = 54;
            gd = 93;
            bd = 143;
            tol = 6;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "taverley") == 0) // Taverley blue dragon
        {
            rd = 16;
            gd = 42;
            bd = 115;
            tol = 6;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "taverley2") == 0) // Taverley blue dragon
        {
            rd = 0;
            gd = 71;
            bd = 109;
            tol = 6;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "mithril") == 0) // mithril dragon
        {
            rd = 75;
            gd = 79;
            bd = 137;
            tol = 6;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "green") == 0) // wilderness green dragon
        {
            rd = 57;
            gd = 111;
            bd = 39;
            tol = 6;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "baby") == 0) // baby blue dragon
        {
            rd = 16;
            gd = 55;
            bd = 116;
            tol = 6;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "steel") == 0) // steel dragon
        {
            rd = 29;
            gd = 93;
            bd = 86;
            tol = 6;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "black") == 0) // black dragon
        {
            rd = 0;
            gd = 0;
            bd = 10;
            tol = 1;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "red") == 0) // red dragon
        {
            rd = 22;
            gd = 21;
            bd = 16;
            tol = 9;
            debug = 1;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "red2") == 0) // red dragon
        {
            rd = 22;
            gd = 21;
            bd = 16;
            tol = 6;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "iron") == 0) // iron dragon
        {
            rd = 18;
            gd = 67;
            bd = 61;
            tol = 3;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else if(strcmp(argv[1], "iron2") == 0) // iron dragon
        {
            rd = 9;
            gd = 47;
            bd = 41;
            tol = 1;
            debug = 0;
            printf("\e[033;33mSet %s auto-hunt target.\e[0m\n", argv[1]);
        }
        else
        {
            printf("\e[033;31mPleb you gave an invalid attack type, now it's set to \"any\" but with degrading scan speed.\e[0m\n");
        }
    }

    if(argc >= 3)
    {
        roam = atoi(argv[2]);
        if(roam < 0)
            roam = 0;
        if(roam > 1)
            roam = 1;
        printf("\e[033;33mSet roam %u.\e[0m\n", roam);
    }

    if(argc >= 4)
    {
        bones = atoi(argv[3]);
        if(bones < 0)
            bones = 0;
        if(bones > 2)
            bones = 2;
        printf("\e[033;33mSet collect drops %u.\e[0m\n", bones);
    }

    if(argc >= 5)
    {
        silent_hunt = atoi(argv[4]);
        if(silent_hunt < 0)
            silent_hunt = 0;
        if(silent_hunt > 1)
            silent_hunt = 1;

        if(debug == 1 && silent_hunt == 1)
        {
            silent_hunt = 0;
            printf("\e[033;31mYou cannot set silent hunt with attack option any.\e[0m\n");
        }
        else
        {
            printf("\e[033;33mSet silent hunt %u.\e[0m\n", silent_hunt);
        }
    }

    if(argc >= 6)
    {
        sm = atof(argv[5]);
        if(sm <= 0)
            sm = 1;
        printf("\e[033;33mSet scan speed %sx.\e[0m\n", argv[5]);
    }

    float base_radius = 54;
    if(argc >= 7)
    {
        base_radius = atof(argv[6]);
        if(base_radius < 1.0)
            base_radius = 1;
        printf("\e[033;33mSet base_radius %spx.\e[0m\n", argv[6]);
    }

    float max_radius = 280;
    if(argc >= 8)
    {
        max_radius = atof(argv[7]);
        if(max_radius < 1.0)
            max_radius = 1;
        printf("\e[033;33mSet max_radius %spx.\e[0m\n", argv[7]);
    }

    printf("\n");

    memset(&event, 0x00, sizeof(event));
    uint flip = 0;
    float angle_step = 0.06 * sm;
    float radius_step = 0.22 * sm;
    float angle = 0;
    float radius = base_radius;
    uint clicks = 0;
    nt = time(0)+qRand(1, 6);

    pthread_t tid;
    if(pthread_create(&tid, NULL, inputThread, NULL) != 0)
    {
        printf("Failed to create the keyscan thread.\n");
        return 0;
    }

    while(1)
    {
        //Loop Delay (1,000 microsecond = 1 millisecond)
        usleep(delaymicro);

        if(key_is_pressed(XK_Z))
            continue;

        //Enable / Disable silent click
        if(enable == 2 && time(0) > nt)
        {
            //Open Display 0
            d = XOpenDisplay((char *) NULL);
            if(d == NULL)
                continue;

            //Get default screen
            si = XDefaultScreen(d);

            //Reset mouse event
            memset(&event, 0x00, sizeof(event));

            //Ready to press down mouse 1
            event.type = ButtonPress;
            event.xbutton.button = Button1;
            event.xbutton.same_screen = True;
            
            //Find target window
            XQueryPointer(d, RootWindow(d, si), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
            event.xbutton.subwindow = event.xbutton.window;
            while(event.xbutton.subwindow)
            {
                event.xbutton.window = event.xbutton.subwindow;
                XQueryPointer(d, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
            }

            //Set starting position for click
            if(lx == 0)
            {
                lx = event.xbutton.x;
                ly = event.xbutton.y;
            }
            ox = event.xbutton.x, oy = event.xbutton.y;
            XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, lx, ly);
            XFlush(d);

            //Fire mouse down
            XSendEvent(d, PointerWindow, True, 0xfff, &event);
            XFlush(d);
            
            //Release mouse down
            event.type = ButtonRelease;
            event.xbutton.state = 0x100;
            XSendEvent(d, PointerWindow, True, 0xfff, &event);
            XFlush(d);

            //Move cursor back
            XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, ox, oy);
            XFlush(d);

            //Next time
            nt = time(0) + qRand(1, 3);
            XCloseDisplay(d);
            continue;
        }
        
        //Enable / Disable entire bot
        if(enable == 1 && time(0) > nt)
        {
            //Open Display 0
            d = XOpenDisplay((char *) NULL);
            if(d == NULL)
                continue;

            //Get default screen
            si = XDefaultScreen(d);

            //Reset mouse event
            memset(&event, 0x00, sizeof(event));

            //Ready to press down mouse 1
            event.type = ButtonPress;
            event.xbutton.button = Button1;
            event.xbutton.same_screen = True;

            //Find target window
            XQueryPointer(d, RootWindow(d, si), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
            event.xbutton.subwindow = event.xbutton.window;
            while(event.xbutton.subwindow)
            {
                event.xbutton.window = event.xbutton.subwindow;
                XQueryPointer(d, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
            }

            //Auto hunt
            if(hunt == 1)
            {
                const int x = radius * cos(angle);
                const int y = radius * sin(angle);

                XWindowAttributes attr;
                XGetWindowAttributes(d, event.xbutton.window, &attr);
                const int cx = attr.width/2;
                const int cy = attr.height/2;

                if(debug == 1)
                {
                    XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, cx + x, cy + y);
                    XFlush(d);
                }

                if(isEnemy(tol, cx + x, cy + y))
                {
                    if(debug == 0)
                    {
                        if(silent_hunt == 1)
                        {
                            ox = event.xbutton.x, oy = event.xbutton.y;
                            XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, lx, ly);
                            XFlush(d);
                        }

                        XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, cx + x, cy + y);
                        XFlush(d);
                    }

                    //Fire mouse down
                    XSendEvent(d, PointerWindow, True, 0xfff, &event);
                    XFlush(d);
                    
                    //Wait between 80 - 220 ms for humanisation
                    if(silent_hunt == 0)
                        usleep(qRand(80000, 220000));
                    
                    //Release mouse down
                    event.type = ButtonRelease;
                    event.xbutton.state = 0x100;
                    XSendEvent(d, PointerWindow, True, 0xfff, &event);
                    XFlush(d);

                    if(debug == 0)
                    {
                        if(silent_hunt == 1)
                        {
                            XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, ox, oy);
                            XFlush(d);
                        }
                    }

                    //Wait between 800 - 3300 ms for humanisation
                    usleep(qRand(800000, 3300000));

                    radius = base_radius;
                    clicks++;
                }

                angle += angle_step * (1-(radius/max_radius));

                if(flip == 0)
                    radius += radius_step;
                else
                    radius -= radius_step;

                if(radius > max_radius || radius < base_radius)
                {
                    radius = base_radius;
                    flip = 0;

                    if(degrade == 1)
                    {
                        angle_step += qRandFloat(0.03, 0.09);//0.06;
                        radius_step += qRandFloat(0.11, 0.34);//0.22;
                        if(angle_step >= 1.0)
                        {
                            angle_step = 0.06 * sm;
                            radius_step = 0.22 * sm;
                        }
                    }
                    
                    //Damn a whole scan and nothing? lets randomly roam
                    if(roam == 1 && clicks == 0)
                    {
                        if(silent_hunt == 1)
                        {
                            ox = event.xbutton.x, oy = event.xbutton.y;
                            XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, lx, ly);
                            XFlush(d);
                        }

                        float na = qRand(0, 58);
                        int px = max_radius * cos(na);
                        int py = max_radius * sin(na);
                        XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, cx+px, cy+py);
                        XFlush(d);
                        while(isBlack() == 1)
                        {
                            na = qRand(0, 58);
                            px = max_radius * cos(na);
                            py = max_radius * sin(na);
                            XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, cx+px, cy+py);
                            XFlush(d);
                            usleep(qRand(80000, 220000));
                        }

                        //Fire mouse down
                        XSendEvent(d, PointerWindow, True, 0xfff, &event);
                        XFlush(d);
                        
                        //Wait between 80 - 220 ms for humanisation
                        if(silent_hunt == 0)
                            usleep(qRand(80000, 220000));
                        
                        //Release mouse down
                        event.type = ButtonRelease;
                        event.xbutton.state = 0x100;
                        XSendEvent(d, PointerWindow, True, 0xfff, &event);
                        XFlush(d);

                        if(silent_hunt == 1)
                        {
                            XWarpPointer(d, None, event.xbutton.window, 0, 0, 0, 0, ox, oy);
                            XFlush(d);
                        }

                        //Next time
                        nt = time(0) + click_delay;
                    }

                    clicks = 0;
                }

                XCloseDisplay(d);
                continue;
            }

            //Get Image Block
            if(event.xbutton.window != twin)
            {
                XCloseDisplay(d);
                continue;
            }
            const uint cmx = 60, cmy = 4;
            XWindowAttributes attr;
            XGetWindowAttributes(d, event.xbutton.window, &attr);
            if(event.xbutton.x >= attr.width-cmx-1 || event.xbutton.y >= attr.height-cmy-1-30 || event.xbutton.x <= cmx+1 || event.xbutton.y <= cmy+1)
            {
                XCloseDisplay(d);
                continue;
            }
            XImage *img = XGetImage(d, event.xbutton.window, event.xbutton.x-30, event.xbutton.y+35, cmx, cmy, AllPlanes, XYPixmap);
            if(img != NULL)
            {
                uint rclick = 0;
                XColor c;
                for(uint x = 0; x < cmx; x++)
                {
                    c.pixel = XGetPixel(img, x, 0);
                    const unsigned char b = c.pixel & img->blue_mask;
                    const unsigned char g = (c.pixel & img->green_mask) >> 8;
                    const unsigned char r = (c.pixel & img->red_mask) >> 16;
                    if(r == 0 && g == 0 && b == 0)
                        rclick++;
                }
                if(rclick == cmx)
                {
                    XCloseDisplay(d);
                    continue;
                }
                
                rclick = 0;
                for(uint y = 1; y < cmy; y++)
                {
                    for(uint x = 0; x < cmx; x++)
                    {
                        c.pixel = XGetPixel(img, x, y);
                        const unsigned char b = c.pixel & img->blue_mask;
                        const unsigned char g = (c.pixel & img->green_mask) >> 8;
                        const unsigned char r = (c.pixel & img->red_mask) >> 16;
                        if(r == 0 && g == 0 && b == 0)
                            rclick++;
                    }
                }
                XFree(img);
                if(rclick == cmx*(cmy-1))
                {
                    //Fire mouse down
                    XSendEvent(d, PointerWindow, True, 0xfff, &event);
                    XFlush(d);
                    
                    //Wait between 80 - 220 ms for humanisation
                    usleep(qRand(80000, 220000));
                    
                    //Release mouse down
                    event.type = ButtonRelease;
                    event.xbutton.state = 0x100;
                    XSendEvent(d, PointerWindow, True, 0xfff, &event);
                    XFlush(d);

                    //Next time
                    nt = time(0);// + 1;
                    usleep(qRand(300000, 600000));

                    XCloseDisplay(d);
                    continue;
                }

            }

            XCloseDisplay(d);
        }
    }

    // never gets here in regular execution flow
    return 0;
}
