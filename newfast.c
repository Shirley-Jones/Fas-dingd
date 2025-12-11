#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <signal.h> 
#include <errno.h>
#include <sys/wait.h>
#include <curl/curl.h>
#define Start_Check_Scripts "./fast.bin"
#define FAS_Version "3.0"
#define Dingd_Name "筑梦工作室 冬瓜"
#define FAS_Name "FAS网络用户管理"  
#define Scripts_Time "2025.12.07 00:00:00"  
/*
--------------------------------------------------------------------

	版权说明: 
	
	流控版权为筑梦网络科技(筑梦工作室)所有！！
	
	FAS流控官网: https://www.dingd.cn 已下线
	
--------------------------------------------------------------------

	脚本说明: 
	
	项目开源地址: https://github.com/Shirley-Jones/FAS-Panel
	
	此版本为最终版本，后续不会对其进行修复或更新。
	
	此脚本由Shirley开源，与筑梦网络科技(筑梦工作室)无关！
	
--------------------------------------------------------------------

	下载地址说明: 
	
	请搜索 raw.githubusercontent.com 您可以快速定位到该位置。
	
	下载地址末尾不加斜杆，否则搭建会报错。
	
	任何问题不要问我，不要问我，不要问我。
	
--------------------------------------------------------------------
*/
char* cmd_system(char* command);
char buff[1024];
int code = 0;
void Readme(void);
void System_Check(void);
void Install_Option(char* IP);
void Install_FAS(char* IP, char* IP_Country);

// 修改WriteCallback，使用结构体来管理动态内存
typedef struct {
    char *data;
    size_t size;
} WriteData;

size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    WriteData *write_data = (WriteData *)userdata;
    
    char *temp = realloc(write_data->data, write_data->size + realsize + 1);
    if (temp == NULL) {
        return 0;
    }
    
    write_data->data = temp;
    memcpy(write_data->data + write_data->size, ptr, realsize);
    write_data->size += realsize;
    write_data->data[write_data->size] = '\0';
    
    return realsize;
}

// 从IPinfo.io获取国家信息
int GetIPCountry(char *ip, char *country, size_t country_size) {
    CURL *curl;
    CURLcode res;
    WriteData write_data = {0};
    char url[256];
    
    // 初始化write_data
    write_data.data = malloc(1);
    if (write_data.data == NULL) {
        return -1;
    }
    write_data.data[0] = '\0';
    write_data.size = 0;

    // 构建查询URL
    snprintf(url, sizeof(url), "https://ipinfo.io/%s/json", ip);
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(write_data.data);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return -1;
        }
        
        // 解析JSON响应获取country字段
        char *country_start = strstr(write_data.data, "\"country\": \"");
        if (country_start != NULL) {
            country_start += 11; // 跳过 "\"country\": \""
            char *country_end = strchr(country_start, '\"');
            if (country_end != NULL) {
                size_t country_len = country_end - country_start;
                if (country_len < country_size) {
                    strncpy(country, country_start, country_len);
                    country[country_len] = '\0';
                }
            }
        }
        
        curl_easy_cleanup(curl);
    }
    
    free(write_data.data);
    curl_global_cleanup();
    return 0;
}

void Obtain_URL_results(char *url, char *ip_buffer, size_t buffer_size) {
    CURL *curl;
    CURLcode res;
    WriteData write_data = {0};
    write_data.data = malloc(1);
    if (write_data.data == NULL) {
        strncpy(ip_buffer, "Error", buffer_size);
        return;
    }
    write_data.data[0] = '\0';
    write_data.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            strncpy(ip_buffer, "Error", buffer_size);
        } else {
            strncpy(ip_buffer, write_data.data, buffer_size);
            ip_buffer[buffer_size - 1] = '\0';
        }
        curl_easy_cleanup(curl);
    } else {
        strncpy(ip_buffer, "Error", buffer_size);
    }
    
    free(write_data.data);
    curl_global_cleanup();
}


char* shellcmd(char* cmd, char* buff, int size)
{
	char temp[256];
	FILE* fp = NULL;
	int offset = 0;
	int len;
	fp = popen(cmd, "r");
	if(fp == NULL)
	{
		return NULL;
	}
 
	while(fgets(temp, sizeof(temp), fp) != NULL)
	{
		len = strlen(temp);
		if(offset + len < size)
		{
			strcpy(buff+offset, temp);
			offset += len;
		}else{
			buff[offset] = 0;
			break;
		}
	}
  
	if(fp != NULL)
	{
		pclose(fp);
	}
	
	return buff;
}
int CentOS_Yum_Install(char* pack)
{
	char co_install[100000];
	sprintf(co_install,"yum install %s -y > /dev/null 2>&1;echo -n $?",pack);
	if(strcat(cmd_system(co_install),"0")!="0"){
		return 1;
	}else{
		return 0;
	}
}

int CentOS_Yum_Uninstall(char* pack)
{
	char co_install[100000];
	sprintf(co_install,"yum remove %s -y > /dev/null 2>&1;echo -n $?",pack);
	if(strcat(cmd_system(co_install),"0")!="0"){
		return 1;
	}else{
		return 0;
	}
}

int Debian_Apt_Install(char* pack)
{
	char co_install[100000];
	sprintf(co_install,"apt-get install %s -y > /dev/null 2>&1;echo -n $?",pack);
	if(strcat(cmd_system(co_install),"0")!="0"){
		return 1;
	}else{
		return 0;
	}
}

int Debian_Apt_Uninstall(char* pack)
{
	char co_install[100000];
	sprintf(co_install,"apt-get remove %s -y > /dev/null 2>&1;echo -n $?",pack);
	if(strcat(cmd_system(co_install),"0")!="0"){
		return 1;
	}else{
		return 0;
	}
}

int runshell(int way,char* content)
{
	/*
	指令说明   by Shirley
	checkcode(runshell(1,""));   CentOS Yum install 安装指令
	checkcode(runshell(2,""));   CentOS Yum remove 卸载指令
	checkcode(runshell(3,""));   Debian apt-get install 安装指令
	checkcode(runshell(4,""));   Debian apt-get remove 卸载指令
	checkcode(runshell(5,""));   直接执行命令
	*/
	
	if(way==1){
		return CentOS_Yum_Install(content);
	}else if(way==2){
		return CentOS_Yum_Uninstall(content);
	}else if(way==3){
		return Debian_Apt_Install(content);
	}else if(way==4){
		return Debian_Apt_Uninstall(content);
	}else if(way==5){
		char com[100000];
		sprintf(com,"%s;echo -n $?",content);
		return atoi(cmd_system(com));
	}else{
		printf("\n程序逻辑错误！脚本终止...\n");
		exit(1);
	}
}

void checkcode(int code1)
{
	if(code1!=0){
	code=code+1;
	}
}

void Progress_bar(pid_t Process_pid)  
{ 	
	int i,j;
	char fh[] = {'-','-','\\','\\','|','|','/','/'};
	while(1){
		for (j = 1; j <= 50; ++j)  
		{  		
			
			printf("\r进度：[ "); 
			for (i = 1; i <= 50; ++i)  
			{  
				if(i==j){
					printf("\033[34m#\033[0m");  
				}else{
					printf("\033[33m-\033[0m");  
				}		
			}  	  	
			printf(" ] [ \033[34m %c Runing...\033[0m ]",fh[j%8]); 
			fflush(stdout);
			usleep(50*1000);
			int Process_status;
			if(waitpid(Process_pid, &Process_status, WNOHANG) == Process_pid){
				printf("\r进度：[ "); 
				for (i = 1; i <= 50; ++i)  
				{ 
					printf("\033[34m#\033[0m");  
				}  	  	
				if(errno != EINTR){
					printf(" ] [ \033[32m    done    \033[0m ]");
				}else{
					printf(" ] [ \033[32m    done    \033[0m ]"); 	
				}
				fflush(stdout);
				return;
			}
		}  	
	}
}  
  
void Start_Progress_bar(char* Process_tip,pid_t Process_pid)  
{
	printf("%s\n",Process_tip); 
	Progress_bar(Process_pid) ;
	printf("\n");
}  


void System_Check()
{
	
	printf("Loading....");
	char Check_Root[10];
	strcpy(Check_Root,cmd_system("echo `whoami` | tr -d '\n'"));
	//检查是否有root权限
	if (!strcmp(Check_Root,"root")==0){
		printf("\n没有ROOT权限，无法搭建！\n");
		exit(0);
    }
	char Linux_OS[500];
	strcpy(Linux_OS,cmd_system("if grep -Eqii \"CentOS\" /etc/issue || grep -Eq \"CentOS\" /etc/*-release; then\n        DISTRO='CentOS'\n        PM='yum'\n    elif grep -Eqi \"Red Hat Enterprise Linux Server\" /etc/issue || grep -Eq \"Red Hat Enterprise Linux Server\" /etc/*-release; then\n        DISTRO='RHEL'\n        PM='yum'\n    elif grep -Eqi \"Aliyun\" /etc/issue || grep -Eq \"Aliyun\" /etc/*-release; then\n        DISTRO='Aliyun'\n        PM='yum'\n    elif grep -Eqi \"Fedora\" /etc/issue || grep -Eq \"Fedora\" /etc/*-release; then\n        DISTRO='Fedora'\n        PM='yum'\n    elif grep -Eqi \"Debian\" /etc/issue || grep -Eq \"Debian\" /etc/*-release; then\n        DISTRO='Debian'\n        PM='apt'\n    elif grep -Eqi \"Ubuntu\" /etc/issue || grep -Eq \"Ubuntu\" /etc/*-release; then\n        DISTRO='Ubuntu'\n        PM='apt'\n    elif grep -Eqi \"Raspbian\" /etc/issue || grep -Eq \"Raspbian\" /etc/*-release; then\n        DISTRO='Raspbian'\n        PM='apt'\n    else\n        DISTRO='未知系统'\n    fi\n    echo $DISTRO | tr -d '\n'"));
	char CentOS_Version[10];
	strcpy(CentOS_Version,cmd_system("echo `cat /etc/redhat-release | awk '{print$4}' | awk -F \".\" '{print$1}'` | tr -d '\n'"));
	//检查Linux发行版本
	if (strcmp(Linux_OS,"CentOS")==0){
		if (strcmp(CentOS_Version,"7")==0){
			
			// 检查所有路径是否都不存在wget
			if (access("/bin/wget", F_OK) != 0 && access("/usr/bin/wget", F_OK) != 0 && access("/sbin/wget", F_OK) != 0 && access("/usr/sbin/wget", F_OK) != 0) {
				checkcode(runshell(1, "wget"));
				if (access("/bin/wget", F_OK) != 0 && access("/usr/bin/wget", F_OK) != 0 && access("/sbin/wget", F_OK) != 0 && access("/usr/sbin/wget", F_OK) != 0) {
					printf("\nwget安装失败，强制退出程序!!!\n");
					exit(1);
				}
			}
			
			// 检查curl是否存在
			if (access("/bin/curl", F_OK) != 0 && access("/usr/bin/curl", F_OK) != 0 && access("/sbin/curl", F_OK) != 0 && access("/usr/sbin/curl", F_OK) != 0) {
				checkcode(runshell(1, "curl"));
				if (access("/bin/curl", F_OK) != 0 && access("/usr/bin/curl", F_OK) != 0 && access("/sbin/curl", F_OK) != 0 && access("/usr/sbin/curl", F_OK) != 0) {
					printf("\ncurl安装失败，强制退出程序!!!\n");
					exit(1);
				}
			}

			// 检查virt-what是否存在
			if (access("/bin/virt-what", F_OK) != 0 && access("/usr/bin/virt-what", F_OK) != 0 && access("/sbin/virt-what", F_OK) != 0 && access("/usr/sbin/virt-what", F_OK) != 0) {
				checkcode(runshell(1, "virt-what"));
				if (access("/bin/virt-what", F_OK) != 0 && access("/usr/bin/virt-what", F_OK) != 0 && access("/sbin/virt-what", F_OK) != 0 && access("/usr/sbin/virt-what", F_OK) != 0) {
					printf("\nvirt-what安装失败，强制退出程序!!!\n");
					exit(1);
				}
			}

			// 检查系统基本命令是否存在
			if ((access("/bin/chmod", F_OK) != 0 && access("/usr/bin/chmod", F_OK) != 0 && access("/sbin/chmod", F_OK) != 0 && access("/usr/sbin/chmod", F_OK) != 0) ||
				(access("/bin/mkdir", F_OK) != 0 && access("/usr/bin/mkdir", F_OK) != 0 && access("/sbin/mkdir", F_OK) != 0 && access("/usr/sbin/mkdir", F_OK) != 0) ||
				(access("/bin/mv", F_OK) != 0 && access("/usr/bin/mv", F_OK) != 0 && access("/sbin/mv", F_OK) != 0 && access("/usr/sbin/mv", F_OK) != 0) ||
				(access("/bin/rm", F_OK) != 0 && access("/usr/bin/rm", F_OK) != 0 && access("/sbin/rm", F_OK) != 0 && access("/usr/sbin/rm", F_OK) != 0) ||
				(access("/bin/cp", F_OK) != 0 && access("/usr/bin/cp", F_OK) != 0 && access("/sbin/cp", F_OK) != 0 && access("/usr/sbin/cp", F_OK) != 0)) {
				printf("\n系统环境异常，强制退出程序!!!\n");
				exit(1);
			}
			
			
		}else{
			printf("\n系统版本验证失败，请更换 CentOS 7\n");
			exit(0);
		}
    }else{
		printf("\n系统版本验证失败，请更换 CentOS 7\n");
		exit(0);
	}
	
	// 检测MySQL是否已安装
	if (!access("/bin/mysql", F_OK) || 
        !access("/usr/bin/mysql", F_OK) || 
        !access("/sbin/mysql", F_OK) || 
        !access("/usr/sbin/mysql", F_OK)) {
        printf("\n错误，检测到您已经安装MySQL Server或已安装FAS流控系统，请先卸载或重装系统后再试！\n");
        exit(1);
    }
	
	// 检查是否安装了其他冲突的服务、程序或流控
	if (!access("/bin/openvpn", F_OK) || 
        !access("/usr/bin/openvpn", F_OK) || 
        !access("/sbin/openvpn", F_OK) ||
		!access("/usr/sbin/openvpn", F_OK) ||
		!access("/var/www/html/system.php", F_OK) || 
		!access("/etc/openvpn/auth_config.conf", F_OK) || 
        !access("/etc/openvpn/server1194.conf", F_OK) || 
		!access("/usr/bin/FasAUTH.bin", F_OK) || 
        !access("/usr/bin/vpn", F_OK)) {
        printf("\n错误，检测到您已经安装OpenVPN或已安装FAS流控系统，请先卸载或重装系统后再试！\n");
        exit(1);
    }
	
	//检查VPS虚拟化
	char Check_VPS[10];
	strcpy(Check_VPS,cmd_system("echo `virt-what` | tr -d '\n'"));
	if (strcmp(Check_VPS,"openvz")==0){
		printf("\n当前VPS的虚拟化是: %s，请更换KVM、Hyper-V虚拟化VPS或物理实体主机！\n",Check_VPS);
		exit(0);
    }
	
	Readme();
}

int Obtain_IP_address() {
    char IP[100] = "";
    char IP_Country[10] = ""; // 存储国家代码
    char *GET_IP_url = "http://members.3322.org/dyndns/getip";
    char Obtain_IP_address_enter[10];

    setbuf(stdout, NULL);
    system("clear");

    printf("\n请稍等\n");
    Obtain_URL_results(GET_IP_url, IP, sizeof(IP));
    IP[strcspn(IP, "\r\n")] = 0;
    
    if (strcmp(IP, "") == 0 || strcmp(IP, "Error") == 0) {
        printf("\n无法检测您的服务器IP，请手动输入IP进行确认！\n");
        sleep(1);
        printf("\n请输入服务器IP: ");
        fgets(IP, sizeof(IP), stdin);
        IP[strcspn(IP, "\r\n")] = 0;

        if (strlen(IP) == 0) {
            printf("\n输入错误，请重新运行脚本\n");
            exit(0);
        } else {
            // 手动选择国家/地区
            int choice;
            printf("\n请选择服务器所在国家/地区：\n");
            printf("1、中国\n");
            printf("2、境外\n");
            printf("请选择[1-2]: ");
            scanf("%d", &choice);
            getchar();
            
            if (choice == 1) {
                strcpy(IP_Country, "CN");
            } else {
                strcpy(IP_Country, "Global");
            }
            
            printf("\n您的IP是: %s 如不正确请立即停止安装，回车继续！", IP);
            fgets(Obtain_IP_address_enter, sizeof(Obtain_IP_address_enter), stdin);
            Install_FAS(IP, IP_Country);
        }
    } else {
        // 自动获取国家信息
        if (GetIPCountry(IP, IP_Country, sizeof(IP_Country)) == 0) {
            // 判断是否为中国大陆
            if (strcmp(IP_Country, "CN") != 0) {
                strcpy(IP_Country, "Global");
            }
        } else {
            // 如果查询失败，默认设为Global
            strcpy(IP_Country, "Global");
        }
        
        printf("\n您的IP是: %s 如不正确请立即停止安装，回车继续！", IP);
        fgets(Obtain_IP_address_enter, sizeof(Obtain_IP_address_enter), stdin);
        Install_FAS(IP, IP_Country);
    }

    return 0;
}


void Readme()
{ 
	char Readme_enter;
	setbuf(stdout,NULL);
	system("clear");
	sleep(1);
	printf("----------------------------------------------------------------\n");
	printf("                  欢迎使用%s系统                   \n",FAS_Name);
	printf("                          版本 V%s                      \n",FAS_Version);
	printf("流控作者: %s \n",Dingd_Name);
	printf("FAS流控官网: https://www.dingd.cn (已下线)  \n");
	printf("\n");
	printf("免责声明：\n");
	printf("此脚本由Shirley开源，与筑梦网络科技(筑梦工作室)无关！\n");
	printf("我们只删除了流控授权部分，其余代码均未修改，原汁原味。\n");
	printf("项目源码已经全部开源，更多说明请查看项目自述文件。\n");
	printf("项目开源地址: https://github.com/Shirley-Jones/Fas-dingd  \n");
	printf("----------------------------------------------------------------\n");
	printf("-----------------------同意 请回车继续--------------------------\n");
    while (1) {
        Readme_enter = getchar();
        if (Readme_enter == '\n') {
            Obtain_IP_address();
            break;
        }
    }
	exit(0);
}



void Install_FAS(char* IP, char* IP_Country) {
	char MySQL_Host[20] = "localhost";
	char MySQL_Port[20] = "3306";
	char MySQL_User[20] = "root";
	char MySQL_Pass[20];
	char SSH_Port[20];
	char Random_MySQL_Pass[20];
	char Download_Host_Select[20];
	char Download_Host[100];
	char Certificate_replacement_status[3];
	char DNS_IP[100];
	if (strcmp(IP_Country, "CN") == 0) {
		strcpy(DNS_IP, "114.114.114.114");
	}else{
		strcpy(DNS_IP, "8.8.8.8");
	}
	
    setbuf(stdout, NULL);
    system("clear");
	
    
	printf("\n开始安装FAS3.0\n");
	sleep(1);
	
	// 随机数据库密码
	strcpy(Random_MySQL_Pass, cmd_system("echo `date +%s%N | md5sum | head -c 20` | tr -d '\n'"));
	
	//获取ssh端口号(centos有效)
	strcpy(SSH_Port, cmd_system("echo `netstat -tulpn | grep sshd | awk '{print $4}' | cut -d: -f2` | tr -d '\n'"));
	
	// 数据库密码
	while (1) {
		printf("\n请设置数据库密码: ");
		fgets(MySQL_Pass, sizeof(MySQL_Pass), stdin);
		MySQL_Pass[strcspn(MySQL_Pass, "\n")] = 0;
		if (strcmp(MySQL_Pass, "") == 0) {
			strcpy(MySQL_Pass, Random_MySQL_Pass);
			printf("已设置数据库密码为: %s\n", MySQL_Pass);
			break;
		} else {
			printf("已设置数据库密码为: %s\n", MySQL_Pass);
			break;
		}
	}
	setbuf(stdout, NULL);
    system("clear");
	printf("\n请选择下载节点");
	printf("\n1.GitHub (Global)");
	printf("\n2.Gitee (China)");
	printf("\n");
	printf("\n请选择[1-2]: ");
	fgets(Download_Host_Select, sizeof(Download_Host_Select), stdin);
    Download_Host_Select[strcspn(Download_Host_Select, "\n")] = 0;
	if (strcmp(Download_Host_Select,"1")==0){
		//资源1地址
		printf("你已选择 1.GitHub (Global)\n");
		strcpy(Download_Host,"https://raw.githubusercontent.com/Shirley-Jones/Fas-dingd/master/resource");
	}else if (strcmp(Download_Host_Select,"2")==0){
		//资源2地址
		printf("你已选择 2.Gitee (China)\n");
		strcpy(Download_Host,"#");
	}else{
		//默认资源地址
		printf("输入无效，系统自动选择 1.GitHub (Global)\n");
		strcpy(Download_Host,"https://gitee.com/JokerPan00/Fas-dingd/raw/master/resource");
	}
	
    printf("\n所需的信息收集完成，即将安装...\n");
	
	sleep(3);
	
	//清屏
	setbuf(stdout,NULL);
	system("clear");
	printf("\n");
	sleep(1);
	
	pid_t Process_pid;
	
	Process_pid = fork();
    if (Process_pid < 0) {
        printf("当前进程出错\n");
        exit(0);
    } else if (Process_pid == 0) {
		checkcode(runshell(5,"setenforce 0 >/dev/null 2>&1"));
        checkcode(runshell(1,"nss telnet avahi openssl openssl-libs openssl-devel lzo lzo-devel pam pam-devel automake pkgconfig gawk tar zip unzip net-tools psmisc gcc pkcs11-helper libxml2 libxml2-devel bzip2 bzip2-devel libcurl libcurl-devel libjpeg libjpeg-devel libpng libpng-devel freetype freetype-devel gmp gmp-devel libmcrypt libmcrypt-devel readline readline-devel libxslt libxslt-devel --skip-broken"));
		checkcode(runshell(1,"epel-release yum-utils"));
		checkcode(runshell(5,"rpm -ivh https://rpms.remirepo.net/enterprise/remi-release-7.rpm >/dev/null 2>&1"));
		checkcode(runshell(5,"yum clean all >/dev/null 2>&1"));
		checkcode(runshell(5,"yum makecache >/dev/null 2>&1"));
		exit(0);
    } else {
        Start_Progress_bar("正在安装系统程序...", Process_pid);
		waitpid(Process_pid, NULL, 0);  // 等待子进程完成
    }
	
	
	
    Process_pid = fork();
    if (Process_pid < 0) {
        printf("当前进程出错\n");
        exit(0);
    } else if (Process_pid == 0) {
		checkcode(runshell(5,"setenforce 0 >/dev/null 2>&1"));
        checkcode(runshell(1,"httpd httpd-tools"));
		checkcode(runshell(1,"--enablerepo=remi --enablerepo=remi-php70 php php-mbstring php-gd php-mysql php-pear php-pear-DB php-cli php-common php-ldap php-odbc php-xmlrpc"));
		checkcode(runshell(5,"sed -i \"s/#ServerName www.example.com:80/ServerName localhost:1024/g\" /etc/httpd/conf/httpd.conf"));
		checkcode(runshell(5,"sed -i \"s/Listen 80/Listen 1024/g\" /etc/httpd/conf/httpd.conf"));
		checkcode(runshell(5,"sed -i \"s/ServerTokens OS/ServerTokens Prod/g\" /etc/httpd/conf/httpd.conf"));
		checkcode(runshell(5,"sed -i \"s/ServerSignature On/ServerSignature Off/g\" /etc/httpd/conf/httpd.conf"));
		checkcode(runshell(5,"sed -i \"s/Options Indexes MultiViews FollowSymLinks/Options MultiViews FollowSymLinks/g\" /etc/httpd/conf/httpd.conf"));
		//checkcode(runshell(5,"sed -i \"s/magic_quotes_gpc = Off/magic_quotes_gpc = On/g\" /etc/php.ini"));
		checkcode(runshell(5,"setsebool httpd_can_network_connect 1 >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl start httpd.service"));
		exit(0);
    } else {
        Start_Progress_bar("正在安装PHP...", Process_pid);
		waitpid(Process_pid, NULL, 0);  // 等待子进程完成
    }
	
	Process_pid = fork();
    if (Process_pid < 0) {
        printf("当前进程出错\n");
        exit(0);
    } else if (Process_pid == 0) {
		checkcode(runshell(1,"mariadb mariadb-server mariadb-devel"));
		checkcode(runshell(5,"systemctl start mariadb.service"));
		char Setup_MySQL_Password[100];
		sprintf(Setup_MySQL_Password,"mysqladmin -uroot password '%s'",MySQL_Pass);
		checkcode(runshell(5,Setup_MySQL_Password));
		char Mkdir_MySQL_DB[100];
		sprintf(Mkdir_MySQL_DB,"mysql -uroot -p%s -e 'create database vpndata;'",MySQL_Pass);
		checkcode(runshell(5,Mkdir_MySQL_DB));
		char Enable_database_remote_access[100];
		sprintf(Enable_database_remote_access,"mysql -uroot -p%s -e \"use mysql;GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY '%s' WITH GRANT OPTION;flush privileges;\"",MySQL_Pass,MySQL_Pass);
		checkcode(runshell(5,Enable_database_remote_access));
		checkcode(runshell(5,"systemctl restart mariadb.service"));
		exit(0);
    } else {
        Start_Progress_bar("正在安装MySQL...", Process_pid);
		waitpid(Process_pid, NULL, 0);  // 等待子进程完成
    }
	
	Process_pid = fork();
    if (Process_pid < 0) {
        printf("当前进程出错\n");
        exit(0);
    } else if (Process_pid == 0) {
		checkcode(runshell(5,"sed -i \"s/SELINUX=enforcing/SELINUX=disabled/g\" /etc/selinux/config"));
		checkcode(runshell(5,"systemctl stop firewalld.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl disable firewalld.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl stop iptables.service >/dev/null 2>&1"));
		checkcode(runshell(1,"iptables iptables-services"));
		checkcode(runshell(5,"systemctl start iptables.service"));
		checkcode(runshell(5,"iptables -F"));
		checkcode(runshell(5,"service iptables save >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl restart iptables.service"));
		checkcode(runshell(5,"iptables -A INPUT -s 127.0.0.1/32  -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -d 127.0.0.1/32  -j ACCEPT"));
		char SSH_Port1[200];
		sprintf(SSH_Port1,"iptables -A INPUT -p tcp -m tcp --dport %s -j ACCEPT",SSH_Port);
		checkcode(runshell(5,SSH_Port1));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 8080 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 443 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 440 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 3389 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 1194 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 1195 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 1196 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 1197 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 80 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 3306 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 138 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 137 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p tcp -m tcp --dport 1024 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p udp -m udp --dport 137 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p udp -m udp --dport 138 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p udp -m udp --dport 53 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -p udp -m udp --dport 5353 -j ACCEPT"));
		checkcode(runshell(5,"iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT"));
		checkcode(runshell(5,"iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING -p udp --dport 138 -j REDIRECT --to-ports 53"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING -p udp --dport 137 -j REDIRECT --to-ports 53"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING -p udp --dport 1194 -j REDIRECT --to-ports 53"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING -p udp --dport 1195 -j REDIRECT --to-ports 53"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING -p udp --dport 1196 -j REDIRECT --to-ports 53"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING -p udp --dport 1197 -j REDIRECT --to-ports 53"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING --dst 10.8.0.1 -p udp --dport 53 -j DNAT --to-destination 10.8.0.1:5353"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING --dst 10.9.0.1 -p udp --dport 53 -j DNAT --to-destination 10.9.0.1:5353"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING --dst 10.10.0.1 -p udp --dport 53 -j DNAT --to-destination 10.10.0.1:5353"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING --dst 10.11.0.1 -p udp --dport 53 -j DNAT --to-destination 10.11.0.1:5353"));
		checkcode(runshell(5,"iptables -t nat -A PREROUTING --dst 10.12.0.1 -p udp --dport 53 -j DNAT --to-destination 10.12.0.1:5353"));
		//checkcode(runshell(5,"iptables -P INPUT DROP"));
		checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE"));
		checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.9.0.0/24 -o eth0 -j MASQUERADE"));
		checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.10.0.0/24 -o eth0 -j MASQUERADE"));
		checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.11.0.0/24 -o eth0 -j MASQUERADE"));
		checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.12.0.0/24 -o eth0 -j MASQUERADE"));
		//checkcode(runshell(5,"iptables -t nat -A POSTROUTING -j MASQUERADE"));
		checkcode(runshell(5,"service iptables save >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl restart iptables.service"));
		checkcode(runshell(5,"echo \"127.0.0.1 localhost\" > /etc/hosts"));
		
        //创建FAS文件夹
		if (access("/FAS",0)){
			checkcode(runshell(5,"mkdir /FAS"));
		}
		
		//下载 FAS Core
		char Download_FAS_Core[100];
		sprintf(Download_FAS_Core,"wget --no-check-certificate -O /FAS/core.zip %s/core.zip >/dev/null 2>&1",Download_Host);
		checkcode(runshell(5,Download_FAS_Core));
		setbuf(stdout,NULL);
		system("cd /FAS && unzip -o core.zip >/dev/null 2>&1");
		checkcode(runshell(5,"chmod -R 0777 /FAS/*"));
		
		//安装OpenVPN
		checkcode(runshell(1,"openvpn"));
		//复制openvpn配置文件
		checkcode(runshell(5,"rm -rf /etc/openvpn"));
		checkcode(runshell(5,"cp -r /FAS/openvpn /etc/openvpn"));
		checkcode(runshell(5,"chmod -R 0777 /etc/openvpn"));
		//检测OpenVPN证书是否到期
		//获取当前时间
		char Current_time[100];
		strcpy(Current_time,cmd_system("echo $(date +%s) | tr -d '\n'"));
		
		//获取证书时间 2027-02-19 00:00:00  1802966400
		char Certificate_time[100];
		strcpy(Certificate_time,"1802966400");
		
		//证书时间-当前时间=剩余有效时间
		
		char Remaining_effective_time[100];
		sprintf(Remaining_effective_time,"echo $(expr %s - %s) | tr -d '\n'",Certificate_time,Current_time);
		char Remaining_effective_time1[100];
		strcpy(Remaining_effective_time1,cmd_system(Remaining_effective_time));
		if (strcmp(Remaining_effective_time1,"0")<=0){
			//小于等于  自动替换新版证书
			if (access("/FAS/new_certificate",0)){
				printf("\n证书文件已被删除，请重新安装FAS。\n");
				exit(1);
			}
			checkcode(runshell(5,"rm -rf /etc/openvpn/easy-rsa/keys"));
			checkcode(runshell(5,"cp -r /FAS/new_certificate /etc/openvpn/easy-rsa/keys"));
			strcpy(Certificate_replacement_status,"yes");
		}else{
			strcpy(Certificate_replacement_status,"no");
		}
		
		//编辑监控文件
		char Edit_OpenVPN_Config_Host[100];
		sprintf(Edit_OpenVPN_Config_Host,"sed -i \"s/远程数据库地址/\"%s\"/g\" /etc/openvpn/auth_config.conf",MySQL_Host);
		checkcode(runshell(5,Edit_OpenVPN_Config_Host));
		
		char Edit_OpenVPN_Config_Port[100];
		sprintf(Edit_OpenVPN_Config_Port,"sed -i \"s/远程数据库端口/\"%s\"/g\" /etc/openvpn/auth_config.conf",MySQL_Port);
		checkcode(runshell(5,Edit_OpenVPN_Config_Port));
		
		char Edit_OpenVPN_Config_User[100];
		sprintf(Edit_OpenVPN_Config_User,"sed -i \"s/远程数据库账户/\"%s\"/g\" /etc/openvpn/auth_config.conf",MySQL_User);
		checkcode(runshell(5,Edit_OpenVPN_Config_User));
		
		char Edit_OpenVPN_Config_Pass[100];
		sprintf(Edit_OpenVPN_Config_Pass,"sed -i \"s/远程数据库密码/\"%s\"/g\" /etc/openvpn/auth_config.conf",MySQL_Pass);
		checkcode(runshell(5,Edit_OpenVPN_Config_Pass));
		
		char Edit_OpenVPN_Config_IP[100];
		sprintf(Edit_OpenVPN_Config_IP,"sed -i \"s/服务器IP/\"%s\"/g\" /etc/openvpn/auth_config.conf",IP);
		checkcode(runshell(5,Edit_OpenVPN_Config_IP));
		
		
		//安装DNSmasq
		checkcode(runshell(1,"dnsmasq"));
		checkcode(runshell(5,"rm -rf /etc/dnsmasq.conf"));
		checkcode(runshell(5,"cp -r /FAS/Config/dnsmasq.conf /etc/dnsmasq.conf"));
		char Edit_DNSmasq_DNS[100];
		sprintf(Edit_DNSmasq_DNS,"sed -i \"s/DNS_Address/\"%s\"/g\" /etc/dnsmasq.conf",DNS_IP);
		checkcode(runshell(5,Edit_DNSmasq_DNS));
		checkcode(runshell(5,"chmod -R 0777 /etc/dnsmasq.conf"));
		checkcode(runshell(5,"echo \"#FAS流控 系统自定义屏蔽host文件 \n\">>/etc/fas_host"));
		checkcode(runshell(5,"chmod -R 0777 /etc/fas_host"));
		
		//复制二进制文件
		checkcode(runshell(5,"cp /FAS/bin/* /usr/bin"));
		checkcode(runshell(5,"cp -r /FAS/res /root/res"));
		
		//复制Sysctl文件
		checkcode(runshell(5,"rm -rf /etc/sysctl.conf"));
		checkcode(runshell(5,"cp /FAS/Config/sysctl.conf /etc/sysctl.conf"));
		checkcode(runshell(5,"sysctl -p >/dev/null 2>&1"));
		
		//创建流量监控文件夹
		if (access("/etc/rate.d",0)){
			checkcode(runshell(5,"mkdir /etc/rate.d"));
			checkcode(runshell(5,"chmod -R 0777 /etc/rate.d/"));
		}
		
		//创建FAS服务开机自启
		checkcode(runshell(5,"cp /FAS/res/fas.service /lib/systemd/system/fas.service"));
		checkcode(runshell(5,"chmod -R 0777 /lib/systemd/system/fas.service"));
		
		//重新加载所有服务
		checkcode(runshell(5,"systemctl daemon-reload >/dev/null 2>&1"));
		
		exit(0);
    } else {
        Start_Progress_bar("正在安装VPN...", Process_pid);
		waitpid(Process_pid, NULL, 0);  // 等待子进程完成
    }
	
	Process_pid = fork();
    if (Process_pid < 0) {
        printf("当前进程出错\n");
        exit(0);
    } else if (Process_pid == 0) {
        checkcode(runshell(5,"rm -rf /var/www/html/*"));
		char Download_FAS_Panel[100];
		sprintf(Download_FAS_Panel,"wget --no-check-certificate -O /var/www/html/web.zip %s/web.zip >/dev/null 2>&1",Download_Host);
		checkcode(runshell(5,Download_FAS_Panel));
		setbuf(stdout,NULL);
		system("cd /var/www/html/ && unzip -o web.zip >/dev/null 2>&1");
		checkcode(runshell(5,"rm -rf /var/www/html/web.zip"));
		char Edit_FAS_Panel[100];
		sprintf(Edit_FAS_Panel,"sed -i \"s/远程数据库地址/\"%s\"/g\" /var/www/html/config.php\nsed -i \"s/远程数据库端口/\"%s\"/g\" /var/www/html/config.php\nsed -i \"s/远程数据库账户/\"%s\"/g\" /var/www/html/config.php\nsed -i \"s/远程数据库密码/\"%s\"/g\" /var/www/html/config.php",MySQL_Host,MySQL_Port,MySQL_User,MySQL_Pass);
		checkcode(runshell(5,Edit_FAS_Panel));
		char Import_Database[100];
		sprintf(Import_Database,"sed -i \"s/服务器IP/\"%s\"/g\" /var/www/html/vpndata.sql\nsed -i \"s/端口/\"1024\"/g\" /var/www/html/vpndata.sql\nmysql -h%s -P%s -u%s -p%s vpndata < /var/www/html/vpndata.sql\nrm -rf /var/www/html/vpndata.sql",IP,MySQL_Host,MySQL_Port,MySQL_User,MySQL_Pass);
		checkcode(runshell(5,Import_Database));
		char Download_MySQL_Panel[100];
		sprintf(Download_MySQL_Panel,"wget --no-check-certificate -O /var/www/html/phpmyadmin.zip %s/phpmyadmin.zip >/dev/null 2>&1",Download_Host);
		checkcode(runshell(5,Download_MySQL_Panel));
		setbuf(stdout,NULL);
		system("cd /var/www/html/ && unzip -o phpmyadmin.zip >/dev/null 2>&1");
		checkcode(runshell(5,"rm -rf /var/www/html/phpmyadmin.zip"));
		checkcode(runshell(5,"chmod -R 0777 /var/www/html/*"));
		exit(0);
    } else {
        Start_Progress_bar("正在安装WEB...",Process_pid);
		waitpid(Process_pid, NULL, 0);  // 等待子进程完成
    }
	
	Process_pid = fork();
    if (Process_pid < 0) {
        printf("当前进程出错\n");
        exit(0);
    } else if (Process_pid == 0) {
        //修改亚洲上海时区
		char Check_time_zone[20];
		strcpy(Check_time_zone,cmd_system("timedatectl | grep \"Asia/Shanghai\""));
		if (strcmp(Check_time_zone,"")==0){
			//没有找到时区亚洲上海 Asia/Shanghai  修改时区为亚洲上海 Asia/Shanghai
			checkcode(runshell(5,"timedatectl set-timezone Asia/Shanghai"));
		}
		checkcode(runshell(5,"dhclient >/dev/null 2>&1"));
		checkcode(runshell(5,"echo \"$RANDOM$RANDOM\">/var/www/auth_key.access"));
		//添加开机自启
		checkcode(runshell(5,"systemctl enable iptables.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable mariadb.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable httpd.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server1194.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server1195.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server1196.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server1197.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server-udp.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable dnsmasq.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable fas.service >/dev/null 2>&1"));
		exit(0);
    } else {
        Start_Progress_bar("正在执行最后的操作...", Process_pid);
		waitpid(Process_pid, NULL, 0);  // 等待子进程完成
    }
	
	setbuf(stdout,NULL);
	system("vpn restart");
	sleep(2);
	char LocalPass[100];
	strcpy(LocalPass,cmd_system("echo `cat /var/www/auth_key.access` | tr -d '\n'"));
	
	setbuf(stdout,NULL);
	system("clear");
	printf("---------------------------------------------\n");
	printf("---------------------------------------------\n");
	printf("恭喜，您已经成功安装%s。\n",FAS_Name);
	printf("后台面板: http://%s:1024/admin/ \n",IP);
	printf("账户: admin  密码: admin  本地密码: %s\n",LocalPass);
	printf("代理面板: http://%s:1024/daili \n",IP);
	printf("---------------------------------------------\n");
	printf("数据库面板: http://%s:1024/phpMyAdmin/ \n",IP);
	printf("数据库地址: %s   数据库端口: %s\n",MySQL_Host,MySQL_Port);
	printf("数据库账户: %s   数据库密码: %s\n",MySQL_User,MySQL_Pass);
	if (strcmp(Certificate_replacement_status,"yes")==0){
		//yes  自动替换新版证书
		printf("---------------------------------------------\n");
		printf("FAS原版OpenVPN证书已到期，已自动为您替换新版证书。\n");
		printf("新版OpenVPN证书由Shirley生成并安装，并且新版OpenVPN证书将于2123.04.19到期，在此期间您可以放心使用。\n");
		printf("新版证书<ca>文件位于：/etc/openvpn/easy-rsa/keys/ca.crt\n");
		printf("新版密钥<tls>文件位于：/etc/openvpn/easy-rsa/keys/ta.key\n");
		printf("你需要在后台管理中修改证书和密钥才能正常使用。\n");
	}
	printf("---------------------------------------------\n");
	printf("常用指令: \n");
	printf("重启VPN vpn restart   \n");
	printf("启动VPN vpn start    \n");
	printf("停止VPN vpn stop   \n");
	printf("---------------------------------------------\n");
	printf("说明: \n");
	printf("部分大厂服务器(如阿里云腾讯云) 会报毒 webshell漏洞文件，请自行查看项目自述文件解决。\n");
	printf("项目开源地址: https://github.com/Shirley-Jones/Fas-dingd \n");
	printf("\n任何问题不要问我，不要问我，不要问我。\n");
	printf("---------------------------------------------\n");
	printf("---------------------------------------------\n");
	exit(0);
}


int main(int argc, char *argv[])  //main 起始变量名  不可修改
{
	//启动验证文件名是否正确
	//创建运行后删除文件
	char Delete_Scripts[200];
	sprintf(Delete_Scripts,"rm -rf %s >/dev/null 2>&1",argv[0]);
	if (!strcmp(argv[0],Start_Check_Scripts)==0){
		//运行后删除源文件
		checkcode(runshell(5,Delete_Scripts));
		//启动文件名不正确，拒绝运行脚本
		printf("无法启动！\n");
		exit(0);
	}else{
		//运行后删除源文件
		checkcode(runshell(5,Delete_Scripts));
		System_Check();
		exit(0);
    }
}

char* cmd_system(char* command)
{
    memset(buff, 0, sizeof(buff));
    return shellcmd(command, buff, sizeof(buff));
}