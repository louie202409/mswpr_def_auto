#include<windows.h>
#include<cstdio>
#include<cstring>
#include<vector>
#include<string>
#include<deque>
#include<list>
#include<queue>
#include<stack>
#include<set>
#include<bitset>
#include<algorithm>
#include<functional>
#include<iterator>
#include<utility>
#include<cmath>
#include<cstdlib>
#include<cctype>
#include<complex>
#include<iomanip>
#include<iostream>
#include<fstream>
#include<sstream>
#include<ctime>
#include<conio.h>
typedef BOOL (WINAPI *SetProcessDPIAwareFunc)(void);

struct Api
{
	void EnableDpiAwareness()
	{
		HMODULE hUser32 = LoadLibraryA("user32.dll");
		if (!hUser32)
			return;
		SetProcessDPIAwareFunc pFunc =
			(SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
		if (pFunc)
			pFunc();
		// 注意：这里不 FreeLibrary，因为进程还要继续用
	}
	enum MouseButton
	{
		MOUSE_LEFT   = 0,
		MOUSE_MIDDLE = 1,
		MOUSE_RIGHT  = 2
	};
	class ScreenCache
	{
	public:
		ScreenCache()
			: m_hdcMem(NULL), m_hBmp(NULL), m_pBits(NULL),
			  m_oldBmp(NULL), m_width(0), m_height(0),
			  m_captured(false)
		{
			m_hdcScreen = GetDC(NULL);
		}
	
		~ScreenCache()
		{
			Release();
			if (m_hdcScreen)
				ReleaseDC(NULL, m_hdcScreen);
		}
	
		// 获取整个屏幕的分辨率
		void InitFullScreen()
		{
			if (m_hdcScreen == NULL)
				return;
	
			m_width  = GetDeviceCaps(m_hdcScreen, HORZRES);
			m_height = GetDeviceCaps(m_hdcScreen, VERTRES);
	
			SetupDIB(m_width, m_height);
		}
	
		// 只缓存指定屏幕矩形（左上角 sx,sy，宽 w，高 h）
		void InitRegion(int sx, int sy, int w, int h)
		{
			if (m_hdcScreen == NULL)
				return;
	
			m_regionX = sx;
			m_regionY = sy;
			m_width   = w;
			m_height  = h;
	
			SetupDIB(w, h);
		}
	
		// 重新从屏幕抓取（刷新缓存）
		bool Capture()
		{
			if (m_hdcMem == NULL)
				return false;
	
			int sx = m_regionX;
			int sy = m_regionY;
	
			// 若未指定区域，默认抓整屏
			if (m_width == GetDeviceCaps(m_hdcScreen, HORZRES) &&
				m_height == GetDeviceCaps(m_hdcScreen, VERTRES))
			{
				sx = 0;
				sy = 0;
			}
	
			BOOL ok = BitBlt(m_hdcMem, 0, 0, m_width, m_height,
							 m_hdcScreen, sx, sy, SRCCOPY);
			m_captured = (ok != FALSE);
			return m_captured;
		}
	
		// 从缓存中读取像素（相对缓存左上角坐标）
		bool GetPixelRGB(int x, int y, BYTE* r, BYTE* g, BYTE* b) const
		{
			if (!m_captured || m_pBits == NULL)
				return false;
			if (x < 0 || y < 0 || x >= m_width || y >= m_height)
				return false;
	
			// DIB 每行按 4 字节对齐；32 位色时 rowBytes = width * 4
			int rowBytes = m_width * 4;
			const BYTE* p = m_pBits + y * rowBytes + x * 4;
	
			// 内存中是 BGR 顺序
			if (b) *b = p[0];
			if (g) *g = p[1];
			if (r) *r = p[2];
			return true;
		}
	
		// 直接从缓存中取 COLORREF（0x00BBGGRR）
		COLORREF GetPixelColor(int x, int y) const
		{
			BYTE r = 0, g = 0, b = 0;
			if (!GetPixelRGB(x, y, &r, &g, &b))
				return CLR_INVALID;
			return RGB(r, g, b);
		}
	
		int Width() const
		{
			return m_width;
		}
		int Height() const
		{
			return m_height;
		}
	
	private:
		void SetupDIB(int w, int h)
		{
			if (m_hdcScreen == NULL || w <= 0 || h <= 0)
				return;
	
			// 清理旧的
			if (m_hBmp)
			{
				if (m_hdcMem && m_oldBmp)
					SelectObject(m_hdcMem, m_oldBmp);
				DeleteObject(m_hBmp);
				m_hBmp = NULL;
				m_oldBmp = NULL;
			}
			if (m_hdcMem)
			{
				DeleteDC(m_hdcMem);
				m_hdcMem = NULL;
			}
	
			m_hdcMem = CreateCompatibleDC(m_hdcScreen);
			if (m_hdcMem == NULL)
				return;
	
			BITMAPINFO bmi;
			memset(&bmi, 0, sizeof(bmi));
			bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth	   = w;
			bmi.bmiHeader.biHeight	  = -h;   // 负数 = 自上而下，行 0 在顶部
			bmi.bmiHeader.biPlanes	  = 1;
			bmi.bmiHeader.biBitCount	= 32;
			bmi.bmiHeader.biCompression = BI_RGB;
	
			void* bits = NULL;
			m_hBmp = CreateDIBSection(m_hdcScreen, &bmi, DIB_RGB_COLORS,
									  &bits, NULL, 0);
			if (m_hBmp == NULL)
				return;
	
			m_pBits = (BYTE*)bits;
			m_oldBmp = (HBITMAP)SelectObject(m_hdcMem, m_hBmp);
		}
	
		void Release()
		{
			if (m_hdcMem && m_oldBmp)
			{
				SelectObject(m_hdcMem, m_oldBmp);
			}
			if (m_hBmp)
			{
				DeleteObject(m_hBmp);
			}
			if (m_hdcMem)
			{
				DeleteDC(m_hdcMem);
			}
			m_hdcMem=NULL;
			m_hBmp=NULL;
			m_pBits=NULL;
			m_oldBmp=NULL;
		}
		HDC	m_hdcScreen;
		HDC	m_hdcMem;
		HBITMAP m_hBmp;
		BYTE* m_pBits;
		HBITMAP m_oldBmp;
		int m_width;
		int m_height;
		int m_regionX;
		int m_regionY;
		bool m_captured;
	};
	ScreenCache* screen;
	bool move_mouse_to(int x,int y)
	{
		return SetCursorPos(x,y)!=false;
	}
	bool mouse_button_down(MouseButton button)
	{
		DWORD flag=0;
		switch(button)
			{
			case MOUSE_LEFT:   flag=MOUSEEVENTF_LEFTDOWN;   break;
			case MOUSE_MIDDLE: flag=MOUSEEVENTF_MIDDLEDOWN; break;
			case MOUSE_RIGHT:  flag=MOUSEEVENTF_RIGHTDOWN;  break;
			default:		   return false;
		}
		mouse_event(flag, 0, 0, 0, 0);
		return true;
	}
	bool mouse_button_up(MouseButton button)
	{
		DWORD flag = 0;
		switch (button)
		{
		case MOUSE_LEFT:   flag = MOUSEEVENTF_LEFTUP;   break;
		case MOUSE_MIDDLE: flag = MOUSEEVENTF_MIDDLEUP; break;
		case MOUSE_RIGHT:  flag = MOUSEEVENTF_RIGHTUP;  break;
		default:		   return false;
		}
		mouse_event(flag, 0, 0, 0, 0);
		return true;
	}
	bool mouse_button_click(MouseButton button)
	{
		if(!mouse_button_down(button))
		{
			return false;
		}
		return mouse_button_up(button);
	}
	
	bool capture()
	{
		return screen->Capture();
	}
	bool get_pixel_rgb(int x,int y,BYTE* r,BYTE* g,BYTE* b)
	{
		return screen->GetPixelRGB(x,y,r,g,b);
	}
	void init()
	{
		printf("[api]: initing...\n");
		EnableDpiAwareness();
		printf("Screen: %d * %d\n",
			   GetDeviceCaps(GetDC(NULL), HORZRES),
			   GetDeviceCaps(GetDC(NULL), VERTRES));
		printf("Physical: %d * %d\n",
			   GetSystemMetrics(SM_CXSCREEN),
			   GetSystemMetrics(SM_CYSCREEN));
		
		screen=new ScreenCache;
		screen->InitFullScreen();
		printf("[api]: init complete\n");
	}
};
Api* api;
struct Auto_program
{
	struct Setting
	{
		int valid;
		int button_up_x,button_up_y;
		int button_down_x,button_down_y;
		int detect_1_x,detect_1_y;
		int detect_2_x,detect_2_y;
		int place_1_x,place_1_y;
		int place_2_x,place_2_y;
		int info_cnt;
		Setting()
		{
			valid=0;
			int button_up_x=button_up_y=0;
			int button_down_x=button_down_y=0;
			int detect_1_x=detect_1_y=0;
			int detect_2_x=detect_2_y=0;
			int place_1_x=place_1_y=0;
			int place_2_x=place_2_y=0;
			info_cnt=0;
		}
		void init()
		{
			;
		}
		bool import()
		{
			std::ifstream in("settings.txt");
			std::string str;
			if(!(in>>str && str=="valid"))
			{
				printf("No settings.txt found!\n");
				valid=0;
				return 0;
			}
			in>>button_up_x>>button_up_y;
			in>>button_down_x>>button_down_y;
			in>>detect_1_x>>detect_1_y;
			in>>detect_2_x>>detect_2_y;
			in>>place_1_x>>place_1_y;
			in>>place_2_x>>place_2_y;
			in>>info_cnt;
			in.close();
			valid=1;
			return 1;
		}
		void output()
		{
			std::ofstream out("settings.txt");
			out<<"valid\n";
			out<<button_up_x<<" "<<button_up_y<<"\n";
			out<<button_down_x<<" "<<button_down_y<<"\n";
			out<<detect_1_x<<" "<<detect_1_y<<"\n";
			out<<detect_2_x<<" "<<detect_2_y<<"\n";
			out<<place_1_x<<" "<<place_1_y<<"\n";
			out<<place_2_x<<" "<<place_2_y<<"\n";
			out<<info_cnt;
			out.close();
		}
		void calc()
		{
			;
		}
	};
	Setting* setting;
	Auto_program()
	{
		;
	}
	void init()
	{
		setting=new Setting;
		printf("[Auto_program]: Init starting\n");
		setting->init();
		setting->import();
		setting->calc();
		printf("[Auto_program]: Init complete\n");
	}
	int get_op_type()
	{
		int ret=-1;
		while(!(ret>=0 && ret<=2))
		{
			printf("What operation would u like to perform?\n");
			printf("0. Quit\n");
			printf("1. Auto play\n");
			printf("2. Settings\n");
			printf("> ");
			scanf("%d",&ret);
		}
		return ret;
	}
	bool have_enemy(int x,int y)
	{
		int tmp=0;
		for(int i=x-10;i<=x+10;i++)
		{
			for(int j=y-10;j<=y+10;j++)
			{
				BYTE r,g,b;
				api->get_pixel_rgb(i,j,&r,&g,&b);
				if(r<=250 && g<=250 && b<=250 && r+g+b>=100 && (r!=245 || g!=238 || b!=222))
				{
					tmp++;
				}
			}
		}
		return tmp>=25;
	}
	void op1_work()
	{
		int rounds=-1;
		int mxval=0;
		printf("Wait until u are ready...\n");
		printf("Make sure https://minesweeper.cn/defense/\n");
		api->capture();
		int cone_pos=0;
		if(have_enemy(setting->place_1_x,setting->place_1_y))
		{
			cone_pos=1;
		}
		else
		{
			cone_pos=2;
		}
		printf("cone is at pos %d\n",cone_pos);
		while(!_kbhit())
		{
			api->capture();
//			api->move_mouse_to(400,770);//now_detect_1
//			api->move_mouse_to(490,740);//now_cone_1
//			api->move_mouse_to(600,770);//now_detect_2
//			api->move_mouse_to(510,770);//now_cone_2
//			api->move_mouse_to(1500,182);//rode cone
//			api->move_mouse_to(1500,290);//x
			if(cone_pos==1)//1
			{
				if(have_enemy(setting->detect_2_x,setting->detect_2_y))//2
				{
					printf("sensed enemy!\n");
					api->move_mouse_to(setting->button_down_x,setting->button_down_y);
					api->mouse_button_click(Api::MOUSE_LEFT);
					Sleep(100);
					api->move_mouse_to(setting->place_1_x,setting->place_1_y);
					api->mouse_button_click(Api::MOUSE_LEFT);
					Sleep(100);
					api->move_mouse_to(setting->button_up_x,setting->button_up_y);
					api->mouse_button_click(Api::MOUSE_LEFT);
					Sleep(100);
					api->move_mouse_to(setting->place_2_x,setting->place_2_y);
					api->mouse_button_click(Api::MOUSE_LEFT);
					cone_pos=2;
				}
			}
			else
			{
				if(have_enemy(setting->detect_1_x,setting->detect_1_y))//2
				{
					printf("sensed enemy!\n");
					api->move_mouse_to(setting->button_down_x,setting->button_down_y);
					api->mouse_button_click(Api::MOUSE_LEFT);
					Sleep(100);
					api->move_mouse_to(setting->place_2_x,setting->place_2_y);
					api->mouse_button_click(Api::MOUSE_LEFT);
					Sleep(100);
					api->move_mouse_to(setting->button_up_x,setting->button_up_y);
					api->mouse_button_click(Api::MOUSE_LEFT);
					Sleep(100);
					api->move_mouse_to(setting->place_1_x,setting->place_1_y);
					api->mouse_button_click(Api::MOUSE_LEFT);
					cone_pos=1;
				}
			}
			Sleep(100);
		}
		printf("Playing finished\n",rounds);
	}
	void op2_setting()
	{
		int input=-1;
		while(1)
		{
			printf("Choose setting\n");
			printf("0. Quit\n");
			printf("1. Button up\n");
			printf("2. Button down\n");
			printf("3. Detect 1\n");
			printf("4. Detect 2\n");
			printf("5. Place 1\n");
			printf("6. Place 2\n");
			printf("7. Info level\n");
			scanf("%d",&input);
			if(input<0 || input>9)
			{
				continue;
			}
			else if(input==0)
			{
				break;
			}
			else if(input>=1 && input<=6)
			{
				int input2=-1;
				while(1)
				{
					printf("Choose op\n");
					int* posx=0;
					int* posy=0;
					if(input==1)
					{
						posx=&(setting->button_up_x);
						posy=&(setting->button_up_y);
					}
					else if(input==2)
					{
						posx=&(setting->button_down_x);
						posy=&(setting->button_down_y);
					}
					else if(input==3)
					{
						posx=&(setting->detect_1_x);
						posy=&(setting->detect_1_y);
					}
					else if(input==4)
					{
						posx=&(setting->detect_2_x);
						posy=&(setting->detect_2_y);
					}
					else if(input==5)
					{
						posx=&(setting->place_1_x);
						posy=&(setting->place_1_y);
					}
					else if(input==6)
					{
						posx=&(setting->place_2_x);
						posy=&(setting->place_2_y);
					}
					else
					{
						printf("nullptr waring!\n");
					}
//					std::cout<<posx<<" "<<posy<<"\n";
					printf("pos: x=%d, y=%d\n",*posx,*posy);
					printf("0. Quit\n");
					printf("1. Show pos\n");
					printf("2. Change pos\n");
					scanf("%d",&input2);
					if(input2<0 || input2>3)
					{
						continue;
					}
					else if(input2==0)
					{
						break;
					}
					else if(input2==1)
					{
						api->move_mouse_to(*posx,*posy);
					}
					else if(input2==2)
					{
						printf("> ");
						scanf("%d %d",posx,posy);
					}
				}
			}
			else if(input==7)
			{
				while(1)
				{
					printf("info cnt: %d\n",setting->info_cnt);
					printf("0. Quit\n");
					printf("1. No info\n");
					printf("2. Least info(max score)\n");
					printf("3. Less info(score every round)\n");
					printf("4. More info(dice numbers, etc.)\n");
					printf("5. Most info(choice, etc.)\n");
					printf("6. Debug info\n");
					int input2=-1;
					scanf("%d",&input2);
					if(input2==0)
					{
						break;
					}
					else if(input2<0 || input2>6)
					{
						continue;
					}
					else
					{
						setting->info_cnt=input2;
						continue;
					}
				}
			}
		}
		setting->output();
	}
	void work()
	{
		while(1)
		{
			int status=get_op_type();
			if(status==-1)
			{
				printf("Auto_program: work(): An error?\n");
				exit(0);
			}
			else if(status==0)
			{
				printf("Ending program...\n");
				break;
			}
			else if(status==1)
			{
				op1_work();
			}
			else if(status==2)
			{
				op2_setting();
			}
		}
	}
};
int main()
{
	srand(time(0));
	api=new Api;
	api->init();
	Auto_program* auto_p=new Auto_program;
	auto_p->init();
	auto_p->work();
	return 0;
}

