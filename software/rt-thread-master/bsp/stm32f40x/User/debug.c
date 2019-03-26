#define LOG_TAG    "debug"

#include "init.h"
#include <string.h>
#include "PID.h"
#include "debug.h"
#include "flash.h"
#include "drv_ano.h"
#include "filter.h"
#include "drv_cpu_temp.h"
/*---------------------- Constant / Macro Definitions -----------------------*/		



/*----------------------- Variable Declarations. -----------------------------*/

extern rt_device_t debug_uart_device;	
extern u8 debug_startup_flag;
extern float  volatge;
enum 
{
		DEBUG_NULL,
		PC_VCAN,
		PC_ANO,
		//������
		
}PC_TOOL;//��λ������

char *debug_tool_name[3]={"NULL","VCAN","ANO"};

u8 debug_tool = PC_ANO; //ɽ�� / ������λ�� ���Ա�־λ
volatile u32 debug_count = 0;

/*-----------------------Debug Thread Begin-----------------------------*/

void debug_send_thread_entry(void* parameter)
{
	
		while(1)
		{

				rt_thread_mdelay(1);
				if( debug_startup_flag == 1)//��debug_uart��ʼ����Ϻ� �Ž�����λ��ͨ��
				{
							
						switch(debug_tool)//ѡ����λ��
						{
								case PC_VCAN: Vcan_Send_Data();break;
								case PC_ANO :	ANO_SEND_StateMachine();break;
								default :break;
						}
				}		
		}
}



int Debug_thread_init(void)
{
	  rt_thread_t debug_send_tid;
		/*������̬�߳�*/
    debug_send_tid = rt_thread_create("debug",			 //�߳�����
                    debug_send_thread_entry,									 //�߳���ں�����entry��
                    RT_NULL,							   //�߳���ں���������parameter��
                    1024,										 //�߳�ջ��С����λ���ֽڡ�byte��
                    30,										 	 //�߳����ȼ���priority��
                    10);										 //�̵߳�ʱ��Ƭ��С��tick��= 100ms

    if (debug_send_tid != RT_NULL){
				rt_thread_startup(debug_send_tid);
		}
		return 0;
}
INIT_APP_EXPORT(Debug_thread_init);

/*-----------------------Debug Thread End-----------------------------*/




/* VCANɽ����λ������ BEGIN */
void Vcan_Send_Cmd(void *wareaddr, unsigned int waresize)
{
		#define CMD_WARE     3
    u8 cmdf[2] = {CMD_WARE, ~CMD_WARE};    //���ڵ��� ʹ�õ�ǰ����
    u8 cmdr[2] = {~CMD_WARE, CMD_WARE};    //���ڵ��� ʹ�õĺ�����

    rt_device_write(debug_uart_device, 0,cmdf, 2);    //�ȷ���ǰ����
    rt_device_write(debug_uart_device, 0,(u8 *)wareaddr, waresize);    //��������
    rt_device_write(debug_uart_device, 0,cmdr, 2);    //���ͺ�����
}


void Vcan_Send_Data(void)
{   
		float temp = 0.0f;
	
		static float list[8]= {0};
		temp =get_cpu_temp();
	
		list[0] = JY901.Euler.Roll; 	//����� Roll 
		list[1] = JY901.Euler.Pitch;  //������ Pitch
		list[2] = JY901.Euler.Yaw; 	  //ƫ���� Yaw
		list[3] = temp;//-(Servo_Duty-Servo_Duty_Md);
		list[4] = KalmanFilter(&temp);//corner_meet_rn;//edge_start[1];//
		list[5] = 90;//get_vol();
		list[6] = 30;	//KalmanFilter(&vol)
		list[7] = 0;	//camera_center;
		
		Vcan_Send_Cmd(list,sizeof(list));
}
/* VCANɽ����λ������  END */




/* debug �������߹ر� */
static int debug(int argc, char **argv)
{
    int result = 0;

    if (argc != 2){
				log_e("Proper Usage: debug on /off");//�÷�:������λ������
				result = -RT_ERROR; 
				goto _exit;  
    }
		if( !strcmp(argv[1],"on") ){ //����Ϊ ɽ����λ�� strcmp ����������� ����0
				debug_startup_flag = 1;
				log_v("debug open\r\n");
		}
		else if( !strcmp(argv[1],"off") ){ //����Ϊ ɽ����λ�� strcmp ����������� ����0
				debug_startup_flag = 0;
				log_v("debug off\r\n");
		}
		else {
				log_e("Proper Usage: debug on /off");//�÷�:������λ������
		}

_exit:
    return result;
}
MSH_CMD_EXPORT(debug,ag: debug on);



/* debug ������λ������ */
static int set_debug_tool(int argc,char **argv)
{
		int result = 0;
    if (argc != 2){
				log_e("Proper Usage: debug_by vcan / ano / null");//�÷�:������λ������
				result = -RT_ERROR;
        goto _exit;
    }

		if( !strcmp(argv[1],"vcan") ){ //����Ϊ ɽ����λ�� strcmp ����������� ����0
				debug_tool = PC_VCAN;
		}

		else if( !strcmp(argv[1],"ano") ){ //����Ϊ ������λ��
				debug_tool = PC_ANO;
		}
		else if( !strcmp(argv[1],"null") ){ //����Ϊ ������λ��
				debug_tool = PC_ANO;
		}
		else {
				log_e("Proper Usage: debug_by vcan / ano / null");//�÷�:������λ������
				goto _exit;
		}
		Flash_Update();
_exit:
    return result;
}
MSH_CMD_EXPORT(set_debug_tool,debug_by vcan / ano / null);
