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
#include <sys/stat.h>

#define OS_RELEASE_FILE "/etc/os-release"
#define UBUNTU_ID "ubuntu"
#define DEBIAN_ID "debian"
#define Start_Check_Scripts "./fast.bin"
#define FAS_Version "3.0"
#define Dingd_Name "筑梦工作室 冬瓜"
#define FAS_Name "FAS网络用户管理"  
#define Scripts_Time "2025.12.08 00:00:00"  

#define SMALL_BUFFER_SIZE 100
#define MEDIUM_BUFFER_SIZE 512
#define LARGE_BUFFER_SIZE 512

// 添加函数声明
char* cmd_system(const char* command);  // 修改：添加const
void checkcode(int code1);  // 提前声明checkcode函数
int runshell(int way, const char* content);  // 修改：添加const
void Readme(void);
void System_Check(void);
void Install_Option(char* IP);
void Install_FAS(char* IP, char* IP_Country);
void remove_colon(char* str);
const char *get_os_version_codename();


char buff[1024];
int code = 0;


// 自定义函数: 获取操作系统的版本代号
const char *get_os_version_codename() {
    FILE *file;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    file = fopen(OS_RELEASE_FILE, "r");
    if (file == NULL) {
        perror("无法打开 /etc/os-release 文件!!!");
        return NULL;
    }

    while ((read = getline(&line, &len, file)) != -1) {
        if (strstr(line, "VERSION_CODENAME=") != NULL) {
            char *codename = strchr(line, '=') + 1;
            codename[strcspn(codename, "\n")] = 0;
            fclose(file);
            free(line);
            return strdup(codename);
        }
    }

    fclose(file);
    if (line) {
        free(line);
    }

    return NULL;
}

// 自定义函数: 清空输入缓冲区
void Clear_Buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// 自定义函数: 创建目录
void create_directory(const char* path, mode_t mode) {
    if (mkdir(path, mode) != 0) {
        perror("文件夹创建失败!!!");
        exit(1);
    }
}

// 自定义函数: 去除字符串中的冒号
void remove_colon(char* str) {
    char* src = str;
    char* dst = str;
    while (*src) {
        if (*src != ':') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

// 自定义函数: 检查当前用户是否有 root 权限
int is_root_user() {
    // 获取当前用户的 UID
    uid_t uid = getuid();
    // root 用户的 UID 是 0
    return (uid == 0);
}

// 自定义函数: 获取系统类型和版本号
int get_os_info(char *os_name, size_t os_name_size, char *os_version, size_t os_version_size) {
    FILE *file = fopen(OS_RELEASE_FILE, "r");
    if (!file) {
        perror("无法打开 /etc/os-release 文件!!!");
        return -1; // 打开文件失败
    }

    char line[256];
    int os_type = 0; // 0: unknown, 1: Ubuntu, 2: Debian
    while (fgets(line, sizeof(line), file)) {
        // 判断系统类型
        if (strstr(line, "ID=") == line) {
            char *id = strchr(line, '=') + 1;
            // 去除换行符和引号
            id[strcspn(id, "\n")] = 0;
            if (id[0] == '"') {
                memmove(id, id + 1, strlen(id));
                id[strlen(id) - 1] = 0;
            }
            if (strcmp(id, UBUNTU_ID) == 0) {
                os_type = 1; // Ubuntu
                strncpy(os_name, "Ubuntu", os_name_size);
            } else if (strcmp(id, DEBIAN_ID) == 0) {
                os_type = 2; // Debian
                strncpy(os_name, "Debian", os_name_size);
            }
        }
        // 获取版本号
        if (strstr(line, "VERSION_ID=") == line) {
            char *version = strchr(line, '=') + 1;
            // 去除换行符和引号
            version[strcspn(version, "\n")] = 0;
            if (version[0] == '"') {
                memmove(version, version + 1, strlen(version));
                version[strlen(version) - 1] = 0;
            }
            strncpy(os_version, version, os_version_size);
        }
    }

    fclose(file);
    return os_type; // 返回系统类型
}

// 自定义函数: 比较版本号
int is_version_supported(const char *os_name, const char *os_version) {
    if (strcmp(os_name, "Ubuntu") == 0) {
        // Ubuntu 版本号格式为 XX.XX（如 20.04）
        float version = atof(os_version);
        if (version >= 20.04) {
            return 1; // 版本符合要求
        } else {
            return 0; // 版本不符合要求
        }
    }
    return -1; // 未知系统
}

// 检查二进制文件是否存在于四个路径中的任意一个
int check_tool_paths(const char *filename, const char *paths[], int path_count) {
    for (int i = 0; i < path_count; i++) {
        char full_path[256]; // 假设路径长度不超过 256
        snprintf(full_path, sizeof(full_path), "%s/%s", paths[i], filename);

        // 检查文件是否存在
        if (access(full_path, F_OK) == 0) {
            return 1; // 文件存在
        }
    }
    return 0; // 文件不存在
}

// 自定义函数，用于检查文件是否存在并处理安装逻辑
void check_tool(const char *filename, const char *package, int can_install) {
    const char *paths[] = {
        "/usr/bin",
        "/usr/sbin",
        "/bin",
        "/sbin"
    };
    int path_count = sizeof(paths) / sizeof(paths[0]);

    // 检查文件是否存在于四个路径中的任意一个
    if (!check_tool_paths(filename, paths, path_count)) {
        if (can_install) {
            // 如果文件不存在且可以安装，尝试安装
            checkcode(runshell(3, package));
            // 安装后再次检查
            if (!check_tool_paths(filename, paths, path_count)) {
                printf("\n%s 安装失败，强制退出程序!!!\n", package);
                exit(1);
            }
        } else {
            // 如果文件不存在且不可安装，直接退出
            printf("\n系统环境异常，强制退出程序!!!\n");
            exit(1);
        }
    }
}

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


char* shellcmd(const char* cmd, char* buff, int size)
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
int CentOS_Yum_Install(const char* pack)
{
	char co_install[100000];
	sprintf(co_install,"yum install %s -y > /dev/null 2>&1;echo -n $?",pack);
	if(strcat(cmd_system(co_install),"0")!="0"){
		return 1;
	}else{
		return 0;
	}
}

int CentOS_Yum_Uninstall(const char* pack)
{
	char co_install[100000];
	sprintf(co_install,"yum remove %s -y > /dev/null 2>&1;echo -n $?",pack);
	if(strcat(cmd_system(co_install),"0")!="0"){
		return 1;
	}else{
		return 0;
	}
}

int Debian_Apt_Install(const char* pack)
{
	char co_install[100000];
	sprintf(co_install,"apt-get install %s -y > /dev/null 2>&1;echo -n $?",pack);
	if(strcat(cmd_system(co_install),"0")!="0"){
		return 1;
	}else{
		return 0;
	}
}

int Debian_Apt_Uninstall(const char* pack)
{
	char co_install[100000];
	sprintf(co_install,"apt-get remove %s -y > /dev/null 2>&1;echo -n $?",pack);
	if(strcat(cmd_system(co_install),"0")!="0"){
		return 1;
	}else{
		return 0;
	}
}

int runshell(int way, const char* content)
{
	/*
	指令说明   by Shirley
	checkcode(runshell(3,""));   CentOS Yum install 安装指令
	checkcode(runshell(2,""));   CentOS Yum remove 卸载指令
	checkcode(runshell(3,""));   Ubuntu Debian apt-get install 安装指令
	checkcode(runshell(4,""));   Ubuntu Debian apt-get remove 卸载指令
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
	// 检查是否有 root 权限
    if (!is_root_user()) {
        printf("\n没有 ROOT 权限，无法搭建！\n");
        exit(0);
    }
	
	char os_name[32];
    char os_version[32];

    // 获取系统信息
    int os_type = get_os_info(os_name, sizeof(os_name), os_version, sizeof(os_version));

    // 如果不是 Ubuntu 或 Debian，直接退出
    if (os_type != 1 && os_type != 2) {
        printf("无法识别的操作系统, 请更换Ubuntu20.04+后重试! \n");
        exit(1); // 系统不符合要求，终止程序
    }

    // 检查版本号是否符合要求
	int is_supported = is_version_supported(os_name, os_version);
	if (is_supported == 0) {
		printf("当前系统: %s, 版本: %s (不支持), 请更换Ubuntu20.04+后重试! \n", os_name, os_version);
		exit(1); // 版本号不符合要求，终止程序
	}
	
	// 检查可安装的工具
    check_tool("wget", "wget", 1);       // wget 可以安装
    check_tool("curl", "curl", 1);       // curl 可以安装
    check_tool("virt-what", "virt-what", 1); // virt-what 可以安装

    // 检查系统关键工具（不可安装）
    check_tool("rm", NULL, 0);           // rm 不可安装
    check_tool("cp", NULL, 0);           // cp 不可安装
    check_tool("mv", NULL, 0);           // mv 不可安装
    check_tool("mkdir", NULL, 0);        // mkdir 不可安装
    check_tool("chmod", NULL, 0);        // chmod 不可安装
	
	
	const char* tun_path = "/dev/net/tun";
    // 检查文件是否存在
    if (access(tun_path, F_OK) != 0) {
        printf("TUN不可用\n");
        exit(1);
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
	char MySQL_Host[50] = "localhost";
	char MySQL_Port[20] = "3306";
	char MySQL_User[50] = "root";
	char MySQL_Pass[100];
	char SSH_Port[20];
	char Random_MySQL_Pass[100];
	char Download_Host_Select[20];
	char Download_Host[300];
	char Certificate_replacement_status[4];
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
	
	//获取ssh端口号(Ubuntu有效)
	strcpy(SSH_Port, cmd_system("echo `sshd -T 2>/dev/null | grep \"^port\" | awk '{print $2}'` | tr -d '\n'"));
	
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
	sleep(1);
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
		strcpy(Download_Host,"https://gitee.com/JokerPan00/Fas-dingd/raw/master/resource");
	}else{
		//默认资源地址
		printf("输入无效，系统自动选择 1.GitHub (Global)\n");
		strcpy(Download_Host,"https://raw.githubusercontent.com/Shirley-Jones/Fas-dingd/master/resource");
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
		checkcode(runshell(5, "sync"));
		checkcode(runshell(5, "echo 1 > /proc/sys/vm/drop_caches"));
		checkcode(runshell(5, "echo 2 > /proc/sys/vm/drop_caches"));
		checkcode(runshell(5, "echo 3 > /proc/sys/vm/drop_caches"));
		
		// 检查是否需要加载APT
		if (access("/root/zero_apt", 0)) {
			//printf("正在加载APT请稍等...\n");
			checkcode(runshell(5, "apt update >/dev/null 2>&1"));
			checkcode(runshell(5, "echo 'Already installed' >> /root/zero_apt"));
		}
		
		// 设置 SELinux 宽容模式
		checkcode(runshell(3, "selinux-utils"));
		checkcode(runshell(5,"setenforce 0 >/dev/null 2>&1"));
		//checkcode(runshell(5, "sed -i \"s/#*\$nrconf{restart} = .*/\$nrconf{restart} = 'a';/g\" /etc/needrestart/needrestart.conf"));
		//checkcode(runshell(5, "needrestart >/dev/null 2>&1"));
		exit(0);
    } else {
        Start_Progress_bar("正在初始化环境...", Process_pid);
		waitpid(Process_pid, NULL, 0);  // 等待子进程完成
    }
	
	
	
	
	Process_pid = fork();
    if (Process_pid < 0) {
        printf("当前进程出错\n");
        exit(0);
    } else if (Process_pid == 0) {
		checkcode(runshell(5,"setenforce 0 >/dev/null 2>&1"));
        checkcode(runshell(3,"make openssl gcc gdb net-tools unzip psmisc wget curl zip vim telnet"));
		checkcode(runshell(3,"telnet openssl libssl-dev automake gawk tar zip unzip net-tools psmisc gcc libxml2 bzip2 libcurl4-openssl-dev libboost-all-dev"));
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
		checkcode(runshell(3, "software-properties-common"));
		checkcode(runshell(5, "add-apt-repository ppa:ondrej/php -y"));
        // 再次更新
		checkcode(runshell(5, "apt update >/dev/null 2>&1"));
		checkcode(runshell(3, "apache2"));
		checkcode(runshell(3, "php7.0 php7.0-cli php7.0-common php7.0-gd php7.0-ldap php7.0-mysql php7.0-odbc php7.0-xmlrpc php7.0-mbstring php7.0-gmp"));
		// 修改 Apache 配置
		checkcode(runshell(5, "sed -i 's/80/1024/g' /etc/apache2/sites-enabled/000-default.conf"));
		checkcode(runshell(5, "sed -i 's/Listen 80/Listen 1024/g' /etc/apache2/ports.conf"));
		checkcode(runshell(5, "sed -i 's/Options Indexes FollowSymLinks/Options FollowSymLinks/g' /etc/apache2/apache2.conf"));
		checkcode(runshell(5, "a2enmod headers >/dev/null 2>&1"));
		//checkcode(runshell(5, "sed -i '/<Directory \/>/a\\Header set Access-Control-Allow-Origin \"*\"' /etc/apache2/apache2.conf"));
		checkcode(runshell(5, "systemctl restart apache2.service"));
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
		checkcode(runshell(3, "mariadb-test mariadb-server mariadb-common mariadb-client"));
        checkcode(runshell(3, "libmariadb-dev"));
        checkcode(runshell(5, "systemctl start mariadb.service"));
        
        // 修改：使用更大的缓冲区并改用snprintf
        char Setup_MySQL_Password[MEDIUM_BUFFER_SIZE];
        snprintf(Setup_MySQL_Password, sizeof(Setup_MySQL_Password), 
                 "mysql -e \"USE mysql;ALTER USER 'root'@'localhost' IDENTIFIED BY '%s';FLUSH PRIVILEGES;\"", MySQL_Pass);
        checkcode(runshell(5,Setup_MySQL_Password));
        
        char Mkdir_MySQL_DB[MEDIUM_BUFFER_SIZE];
        snprintf(Mkdir_MySQL_DB, sizeof(Mkdir_MySQL_DB),
                 "mysql -h127.0.0.1 -P3306 -uroot -p%s -e 'create database vpndata;'", MySQL_Pass);
        checkcode(runshell(5,Mkdir_MySQL_DB));
        
        // 修改：使用更大的缓冲区
        char Enable_database_remote_access[LARGE_BUFFER_SIZE];
        snprintf(Enable_database_remote_access, sizeof(Enable_database_remote_access),
                 "mysql -h127.0.0.1 -P3306 -uroot -p%s -e \"use mysql;GRANT ALL PRIVILEGES ON *.* TO 'root'@'%%' IDENTIFIED BY '%s' WITH GRANT OPTION;flush privileges;\"", 
                 MySQL_Pass, MySQL_Pass);
        checkcode(runshell(5,Enable_database_remote_access));
        
        checkcode(runshell(5, "echo '[mysqld]\nbind-address = 0.0.0.0' >> /etc/mysql/my.cnf"));
        checkcode(runshell(5, "systemctl restart mariadb.service"));
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
       //创建FAS文件夹
        if (access("/FAS",0)){
            create_directory("/FAS", 0777);
        }
        
        //下载 FAS Core - 修改：使用更大的缓冲区
        char Download_FAS_Core[MEDIUM_BUFFER_SIZE];
        snprintf(Download_FAS_Core, sizeof(Download_FAS_Core),
                 "wget --no-check-certificate -O /FAS/core_ubuntu.zip %s/core_ubuntu.zip >/dev/null 2>&1", Download_Host);
        checkcode(runshell(5,Download_FAS_Core));
        
		checkcode(runshell(5,"unzip -o /FAS/core_ubuntu.zip -d /FAS >/dev/null 2>&1"));
        checkcode(runshell(5,"chmod -R 0777 /FAS/*"));
        
		
		//修改apache2服务名
		checkcode(runshell(5,"sed -i 's/httpd.service/apache2.service/g' /FAS/bin/vpn"));
		//禁用iptables(Ubuntu系统并不是通过service管理iptables!!!)
		checkcode(runshell(5,"sed -i 's/systemctl restart iptables.service/#systemctl restart iptables.service/g' /FAS/bin/vpn"));
		checkcode(runshell(5,"sed -i 's/systemctl stop iptables.service/#systemctl stop iptables.service/g' /FAS/bin/vpn"));
		checkcode(runshell(5,"sed -i 's/systemctl start iptables.service/#systemctl start iptables.service/g' /FAS/bin/vpn"));
		
		// 修改：使用更大的缓冲区
        char Edit_DNSmasq_DNS[MEDIUM_BUFFER_SIZE];
        snprintf(Edit_DNSmasq_DNS, sizeof(Edit_DNSmasq_DNS),
                 "sed -i \"s/DNS_Address/\"%s\"/g\" /FAS/Config/dnsmasq.conf", DNS_IP);
        checkcode(runshell(5,Edit_DNSmasq_DNS));
		
        //安装OpenVPN
        checkcode(runshell(3,"openvpn"));
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
        // 修改：使用更大的缓冲区
        char Remaining_effective_time[MEDIUM_BUFFER_SIZE];
        snprintf(Remaining_effective_time, sizeof(Remaining_effective_time),
                 "echo $(expr %s - %s) | tr -d '\n'", Certificate_time, Current_time);
        
        char Remaining_effective_time1[100];
        strcpy(Remaining_effective_time1,cmd_system(Remaining_effective_time));
        if (strcmp(Remaining_effective_time1,"0")<=0){
            //小于等于  自动替换新版证书
            checkcode(runshell(5,"rm -rf /etc/openvpn/easy-rsa/keys"));
            checkcode(runshell(5,"cp -r /FAS/new_certificate /etc/openvpn/easy-rsa/keys"));
            // 修改：使用安全的字符串拷贝
            strncpy(Certificate_replacement_status, "yes", sizeof(Certificate_replacement_status)-1);
            Certificate_replacement_status[sizeof(Certificate_replacement_status)-1] = '\0';
        }else{
            strncpy(Certificate_replacement_status, "no", sizeof(Certificate_replacement_status)-1);
            Certificate_replacement_status[sizeof(Certificate_replacement_status)-1] = '\0';
        }
		
		//安装DNSmasq
        checkcode(runshell(3,"dnsmasq"));
        checkcode(runshell(5,"rm -rf /etc/dnsmasq.conf"));
        checkcode(runshell(5,"cp -r /FAS/Config/dnsmasq.conf /etc/dnsmasq.conf"));
		checkcode(runshell(5,"chmod -R 0777 /etc/dnsmasq.conf"));
        checkcode(runshell(5,"echo \"#FAS流控 系统自定义屏蔽host文件 \n\">>/etc/fas_host"));
        checkcode(runshell(5,"chmod -R 0777 /etc/fas_host"));
		
		
		checkcode(runshell(3,"iptables"));
        checkcode(runshell(5,"iptables -A INPUT -s 127.0.0.1/32 -j ACCEPT"));
        checkcode(runshell(5,"iptables -A INPUT -d 127.0.0.1/32 -j ACCEPT"));
        
        char SSH_Port1[MEDIUM_BUFFER_SIZE];
        snprintf(SSH_Port1, sizeof(SSH_Port1),"iptables -A INPUT -p tcp -m tcp --dport %s -j ACCEPT",SSH_Port);
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
        checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE"));
        checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.9.0.0/24 -o eth0 -j MASQUERADE"));
        checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.10.0.0/24 -o eth0 -j MASQUERADE"));
        checkcode(runshell(5,"iptables -t nat -A POSTROUTING -s 10.11.0.0/24 -o eth0 -j MASQUERADE"));
        checkcode(runshell(5,"echo '127.0.0.1 localhost' > /etc/hosts"));
        checkcode(runshell(5,"iptables-save > /etc/openvpn/fas_rules.v4"));
		
        
        //编辑监控文件 - 修改：使用更大的缓冲区
        char Edit_OpenVPN_Config_Host[MEDIUM_BUFFER_SIZE];
        snprintf(Edit_OpenVPN_Config_Host, sizeof(Edit_OpenVPN_Config_Host),
                 "sed -i \"s/远程数据库地址/\"%s\"/g\" /etc/openvpn/auth_config.conf", MySQL_Host);
        checkcode(runshell(5,Edit_OpenVPN_Config_Host));
        
        char Edit_OpenVPN_Config_Port[MEDIUM_BUFFER_SIZE];
        snprintf(Edit_OpenVPN_Config_Port, sizeof(Edit_OpenVPN_Config_Port),
                 "sed -i \"s/远程数据库端口/\"%s\"/g\" /etc/openvpn/auth_config.conf", MySQL_Port);
        checkcode(runshell(5,Edit_OpenVPN_Config_Port));
        
        char Edit_OpenVPN_Config_User[MEDIUM_BUFFER_SIZE];
        snprintf(Edit_OpenVPN_Config_User, sizeof(Edit_OpenVPN_Config_User),
                 "sed -i \"s/远程数据库账户/\"%s\"/g\" /etc/openvpn/auth_config.conf", MySQL_User);
        checkcode(runshell(5,Edit_OpenVPN_Config_User));
        
        char Edit_OpenVPN_Config_Pass[MEDIUM_BUFFER_SIZE];
        snprintf(Edit_OpenVPN_Config_Pass, sizeof(Edit_OpenVPN_Config_Pass),
                 "sed -i \"s/远程数据库密码/\"%s\"/g\" /etc/openvpn/auth_config.conf", MySQL_Pass);
        checkcode(runshell(5,Edit_OpenVPN_Config_Pass));
        
        char Edit_OpenVPN_Config_IP[MEDIUM_BUFFER_SIZE];
        snprintf(Edit_OpenVPN_Config_IP, sizeof(Edit_OpenVPN_Config_IP),
                 "sed -i \"s/服务器IP/\"%s\"/g\" /etc/openvpn/auth_config.conf", IP);
        checkcode(runshell(5,Edit_OpenVPN_Config_IP));
        
        //编译组件
        checkcode(runshell(5,"gcc -o /FAS/bin/FasAUTH.bin /FAS/source_code/Shirley_FasAUTH.c -lmariadbclient -lcurl -lcrypto > /dev/null 2>&1"));
        checkcode(runshell(5,"chmod -R 0777 /FAS/bin/FasAUTH.bin"));
        checkcode(runshell(5,"gcc -o /FAS/bin/openvpn.bin /FAS/source_code/openvpn.c > /dev/null 2>&1"));
        checkcode(runshell(5,"chmod -R 0777 /FAS/bin/openvpn.bin"));
        checkcode(runshell(5,"gcc -o /FAS/bin/rate.bin /FAS/source_code/Rate.c > /dev/null 2>&1"));
        checkcode(runshell(5,"chmod -R 0777 /FAS/bin/rate.bin"));
        checkcode(runshell(5,"gcc -o /FAS/res/proxy.bin /FAS/source_code/Proxy.c > /dev/null 2>&1"));
        checkcode(runshell(5,"chmod -R 0777 /FAS/res/proxy.bin"));
        checkcode(runshell(5,"gcc -o /FAS/res/fas-service /FAS/source_code/Socket.c -lcrypto > /dev/null 2>&1"));
        checkcode(runshell(5,"chmod -R 0777 /FAS/res/fas-service"));
        
        //复制二进制文件
        checkcode(runshell(5,"cp /FAS/bin/* /usr/bin"));
        checkcode(runshell(5,"cp -r /FAS/res /root/res"));
        
        //复制Sysctl文件
        checkcode(runshell(5,"rm -rf /etc/sysctl.conf"));
        checkcode(runshell(5,"cp /FAS/Config/sysctl.conf /etc/sysctl.conf"));
        checkcode(runshell(5,"sysctl -p >/dev/null 2>&1"));
        
        //创建网络流量文件夹
        if (access("/etc/rate.d",0)){
			create_directory("/etc/rate.d", 0777);
        }
        
        //创建FAS服务开机自启
        checkcode(runshell(5,"cp /FAS/res/fas.service /lib/systemd/system/fas.service"));
        checkcode(runshell(5,"chmod -R 0777 /lib/systemd/system/fas.service"));
        checkcode(runshell(5,"cp /FAS/res/auto_run.service /lib/systemd/system/auto_run.service"));
        checkcode(runshell(5,"chmod -R 0777 /lib/systemd/system/auto_run.service"));
        
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
        // 修改：使用更大的缓冲区
        char Download_FAS_Panel[MEDIUM_BUFFER_SIZE];
        snprintf(Download_FAS_Panel, sizeof(Download_FAS_Panel),
                 "wget --no-check-certificate -O /var/www/html/web.zip %s/web.zip >/dev/null 2>&1", Download_Host);
        checkcode(runshell(5,Download_FAS_Panel));
        
		checkcode(runshell(5,"unzip -o /var/www/html/web.zip -d /var/www/html >/dev/null 2>&1"));
        checkcode(runshell(5,"rm -rf /var/www/html/web.zip"));
        
        // 修改：使用更大的缓冲区
        char Edit_FAS_Panel[LARGE_BUFFER_SIZE];
        snprintf(Edit_FAS_Panel, sizeof(Edit_FAS_Panel),
                 "sed -i \"s/远程数据库地址/\"%s\"/g\" /var/www/html/config.php\n"
                 "sed -i \"s/远程数据库端口/\"%s\"/g\" /var/www/html/config.php\n"
                 "sed -i \"s/远程数据库账户/\"%s\"/g\" /var/www/html/config.php\n"
                 "sed -i \"s/远程数据库密码/\"%s\"/g\" /var/www/html/config.php", 
                 MySQL_Host, MySQL_Port, MySQL_User, MySQL_Pass);
        checkcode(runshell(5,Edit_FAS_Panel));
		
		// 修改：使用更大的缓冲区
		char Import_Database[LARGE_BUFFER_SIZE];
		snprintf(Import_Database, sizeof(Import_Database),
				"sed -i \"s/服务器IP/\"%s\"/g\" /var/www/html/vpndata.sql\n"
				"sed -i \"s/端口/\"1024\"/g\" /var/www/html/vpndata.sql\n"
				"mysql -h%s -P%s -u%s -p%s vpndata < /var/www/html/vpndata.sql\n"
				"rm -rf /var/www/html/vpndata.sql",
				IP,MySQL_Host,MySQL_Port,MySQL_User,MySQL_Pass);
		checkcode(runshell(5,Import_Database));
		
		// 修改：使用更大的缓冲区
		char Download_MySQL_Panel[LARGE_BUFFER_SIZE];
		snprintf(Download_MySQL_Panel, sizeof(Download_MySQL_Panel),
				"wget --no-check-certificate -O /var/www/html/phpmyadmin.zip %s/phpmyadmin.zip >/dev/null 2>&1",Download_Host);
		checkcode(runshell(5,Download_MySQL_Panel));
		checkcode(runshell(5,"unzip -o /var/www/html/phpmyadmin.zip -d /var/www/html >/dev/null 2>&1"));
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
		//checkcode(runshell(5,"dhclient >/dev/null 2>&1"));
		checkcode(runshell(5,"echo \"$RANDOM$RANDOM\">/var/www/auth_key.access"));
		//添加开机自启
		checkcode(runshell(5,"systemctl enable mariadb.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable apache2.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server1194.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server1195.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server1196.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server1197.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable openvpn@server-udp.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable dnsmasq.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable fas.service >/dev/null 2>&1"));
		checkcode(runshell(5,"systemctl enable auto_run.service >/dev/null 2>&1"));
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
		printf("Loading....\n");
		//运行后删除源文件
		checkcode(runshell(5,Delete_Scripts));
		System_Check();
		exit(0);
    }
}

char* cmd_system(const char* command)
{
    memset(buff, 0, sizeof(buff));
    return shellcmd(command, buff, sizeof(buff));
}