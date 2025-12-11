#!/bin/bash
#FAS流控引导脚本
#项目地址: https://github.com/Shirley-Jones/Fas-dingd
#版权说明: 
#流控版权为筑梦网络科技(筑梦工作室)所有！！
#此脚本由Shirley开源，与筑梦网络科技(筑梦工作室)无关！
#FAS流控官网: https://www.dingd.cn 已下线
#此版本为最终版本，后续不会对其进行修复或更新。
#兼容Ubuntu系统


Install_FAS()
{
	clear
	echo 
	
	# 获取服务器的公网IP地址
	SERVER_IP=$(curl -s http://members.3322.org/dyndns/getip)
	if [ -z "$SERVER_IP" ]; then
		echo "错误：无法获取服务器IP地址"
		exit 1
	fi

	echo "检测到服务器公网IP: $SERVER_IP"
	# 使用ipinfo.io API获取国家代码
	COUNTRY_CODE=$(curl -s "https://ipinfo.io/$SERVER_IP/json" | grep -o '"country": *"[^"]*' | cut -d'"' -f4)
	if [ -z "$COUNTRY_CODE" ]; then
		echo "警告：无法获取国家代码，默认使用Github节点"
		COUNTRY_CODE="US"
	fi

	echo "检测到地区代码: $COUNTRY_CODE"

	# 节点配置
	if [[ ${Linux_OS} ==  "CentOS" ]]; then
		# 支持的
		if [[ !${Linux_Version} ==  "7" ]]; then 
			echo "当前Linux系统不支持安装FAS流控,请更换系统后重新尝试."
			exit 1;
		else
			#支持的
			GITHUB_NODE="https://raw.githubusercontent.com/Shirley-Jones/Fas-dingd/master/fast.bin"
			GITEE_NODE="https://gitee.com/JokerPan00/Fas-dingd/raw/master/fast.bin"
		fi
	elif [[ ${Linux_OS} ==  "Ubuntu" ]]; then
		# 支持的
		result=$(echo "$Linux_Version < 20.04" | bc -l)
		if [ $result -eq 1 ]; then
			echo "当前Linux系统不支持安装FAS流控,请更换系统后重新尝试."
			exit 1;
		else
			GITHUB_NODE="https://raw.githubusercontent.com/Shirley-Jones/Fas-dingd/master/fast_ubuntu.bin"
			GITEE_NODE="https://gitee.com/JokerPan00/Fas-dingd/raw/master/fast_ubuntu.bin"
		fi
	else
		echo "当前的Linux系统不支持安装FAS流控,请更换系统后重新尝试."
		exit 1;
	fi
	
	# 根据国家代码选择节点
	if [ "$COUNTRY_CODE" = "CN" ]; then
		echo "服务器位于中国，自动选择Gitee节点"
		DOWNLOAD_URL="$GITEE_NODE"
	else
		echo "服务器位于海外，自动选择Github节点"
		DOWNLOAD_URL="$GITHUB_NODE"
	fi

	# 下载文件
	echo "开始下载: $DOWNLOAD_URL"
	# 完整的下载路径
	FULL_PATH="/root/fast.bin"

	wget --tries=3 --timeout=30 --waitretry=5 --progress=bar -O "$FULL_PATH" "$DOWNLOAD_URL"

	# 检查下载结果
	if [ $? -eq 0 ]; then
		echo "下载成功: $FULL_PATH"
		chmod -R 0777 /root/fast.bin
		cd /root
		./fast.bin
		exit 0;
	else
		echo "错误：下载失败"
		# 清理可能的部分下载文件
		if [ -f "$FULL_PATH" ]; then
			rm -rf "$FULL_PATH"
		fi
		exit 1
	fi


	exit 0;
}


Cluster_load()
{
	clear
	echo
	read -p "请输入主机/云数据库地址: " Database_Address
	while [[ ${Database_Address} == "" ]]
	do
		echo "\033[31m检测到数据库地址没有输入，请重新尝试！\033[0m"
		read -p "请输入主机/云数据库地址: " Database_Address
	done

	echo
	read -p "请输入主机/云数据库端口: " Database_Port
	while [[ ${Database_Port} == "" ]]
	do
		echo "\033[31m检测到数据库端口没有输入，请重新尝试！\033[0m"
		read -p "请输入主机/云数据库端口: " Database_Port
	done

	echo
	read -p "请输入主机/云数据库账户: " Database_Username
	while [[ ${Database_Username} == "" ]]
	do
		echo "\033[31m检测到数据库账户没有输入，请重新尝试！\033[0m"
		read -p "请输入主机/云数据库账户: " Database_Username
	done

	echo
	read -p "请输入主机/云数据库密码: " Database_Password
	while [[ ${Database_Password} == "" ]]
	do
		echo "\033[31m检测到数据库密码没有输入，请重新尝试！\033[0m"
		read -p "请输入主机/云数据库密码: " Database_Password
	done

	echo
	echo "开始修改配置文件..."

	# 修改第一个文件：/etc/openvpn/auth_config.conf
	if [ -f "/etc/openvpn/auth_config.conf" ]; then
		echo "正在修改 /etc/openvpn/auth_config.conf"
		
		# 使用sed命令替换数据库配置
		sed -i "s/^mysql_host=\".*\"/mysql_host=\"${Database_Address}\"/" /etc/openvpn/auth_config.conf
		sed -i "s/^mysql_port=\".*\"/mysql_port=\"${Database_Port}\"/" /etc/openvpn/auth_config.conf
		sed -i "s/^mysql_user=\".*\"/mysql_user=\"${Database_Username}\"/" /etc/openvpn/auth_config.conf
		sed -i "s/^mysql_pass=\".*\"/mysql_pass=\"${Database_Password}\"/" /etc/openvpn/auth_config.conf
	else
		echo "警告: /etc/openvpn/auth_config.conf 文件不存在,修改失败,脚本退出."
		exit 1;
	fi

	# 修改第二个文件：/var/www/html/config.php
	if [ -f "/var/www/html/config.php" ]; then
		echo "正在修改 /var/www/html/config.php"
		
		# 使用sed命令替换数据库配置
		sed -i "s/define(\"_host_\",\".*\");/define(\"_host_\",\"${Database_Address}\");/" /var/www/html/config.php
		sed -i "s/define(\"_port_\",\".*\");/define(\"_port_\",\"${Database_Port}\");/" /var/www/html/config.php
		sed -i "s/define(\"_user_\",\".*\");/define(\"_user_\",\"${Database_Username}\");/" /var/www/html/config.php
		sed -i "s/define(\"_pass_\",\".*\");/define(\"_pass_\",\"${Database_Password}\");/" /var/www/html/config.php
	else
		echo "警告: /var/www/html/config.php 文件不存在,修改失败,脚本退出."
	fi

	echo
	echo "数据库配置修改完成！"
	echo "修改摘要："
	echo "数据库地址: ${Database_Address}"
	echo "数据库端口: ${Database_Port}"
	echo "数据库用户: ${Database_Username}"
	echo "数据库密码: ${Database_Password}"
	echo "正在重启VPN服务请稍等..."
	sleep 3
	/usr/bin/vpn restart
	exit 0;
}



Reset_iptables()
{
	echo
	read -p "是否重置防火墙? [Y/N] " yn
	case $yn in
		[yY]) 
			echo "正在操作中..."
			if [[ ${Linux_OS} ==  "CentOS" ]]; then
				# 支持的
				yum install iptables iptables-services -y >/dev/null 2>&1
				SSH_Port=$(netstat -tulpn | grep sshd | awk '{print $4}' | cut -d: -f2);
				Apache_Port=$(awk '/^Listen/ {print $2}' /etc/httpd/conf/httpd.conf);
			elif [[ ${Linux_OS} ==  "Ubuntu" ]]; then
				# 支持的
				apt update >/dev/null 2>&1
				apt install iptables -y >/dev/null 2>&1
				port=$(sshd -T 2>/dev/null | grep "^port" | awk '{print $2}')
				if [[ -z "$port" ]]; then
					SSH_Port=$(netstat -tulpn 2>/dev/null | grep sshd | head -1 | awk '{print $4}' | cut -d: -f2)
				else
					SSH_Port=${port}
				fi
				if [ -f "/etc/apache2/ports.conf" ]; then
					#读取配置文件
					Apache_Port=$(grep -E 'Listen [0-9]+$' /etc/apache2/ports.conf | awk '{print $2}' | head -n 1)
				else
					Apache_Port="1024";
				fi
			else
				echo "程序逻辑错误,脚本已被终止..."
				exit 1;
			fi
			iptables -F
			iptables -P INPUT ACCEPT
			iptables -P FORWARD ACCEPT
			iptables -P OUTPUT ACCEPT
			iptables -t nat -P PREROUTING ACCEPT
			iptables -t nat -P POSTROUTING ACCEPT
			iptables -t nat -P OUTPUT ACCEPT
			iptables -t nat -F
			iptables -X
			iptables -t nat -X
			iptables -A INPUT -s 127.0.0.1/32  -j ACCEPT
			iptables -A INPUT -d 127.0.0.1/32  -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport ${SSH_Port} -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport ${Apache_Port} -j ACCEPT
			#proxy端口
			cat /root/res/portlist.conf | while read Proxy_TCP_Port_List
			do
				Proxy_TCP_Port=`echo $Proxy_TCP_Port_List | cut -d \  -f 2`
				iptables -A INPUT -p tcp -m tcp --dport $Proxy_TCP_Port -j ACCEPT
			done
			iptables -A INPUT -p tcp -m tcp --dport 8080 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 443 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 440 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 3389 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 1194 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 1195 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 1196 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 1197 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 80 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 3306 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 138 -j ACCEPT
			iptables -A INPUT -p tcp -m tcp --dport 137 -j ACCEPT
			iptables -A INPUT -p udp -m udp --dport 137 -j ACCEPT
			iptables -A INPUT -p udp -m udp --dport 138 -j ACCEPT
			iptables -A INPUT -p udp -m udp --dport 53 -j ACCEPT
			iptables -A INPUT -p udp -m udp --dport 5353 -j ACCEPT
			iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
			iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
			iptables -t nat -A PREROUTING -p udp --dport 138 -j REDIRECT --to-ports 53
			iptables -t nat -A PREROUTING -p udp --dport 137 -j REDIRECT --to-ports 53
			iptables -t nat -A PREROUTING -p udp --dport 1194 -j REDIRECT --to-ports 53
			iptables -t nat -A PREROUTING -p udp --dport 1195 -j REDIRECT --to-ports 53
			iptables -t nat -A PREROUTING -p udp --dport 1196 -j REDIRECT --to-ports 53
			iptables -t nat -A PREROUTING -p udp --dport 1197 -j REDIRECT --to-ports 53
			iptables -t nat -A PREROUTING --dst 10.8.0.1 -p udp --dport 53 -j DNAT --to-destination 10.8.0.1:5353
			iptables -t nat -A PREROUTING --dst 10.9.0.1 -p udp --dport 53 -j DNAT --to-destination 10.9.0.1:5353
			iptables -t nat -A PREROUTING --dst 10.10.0.1 -p udp --dport 53 -j DNAT --to-destination 10.10.0.1:5353
			iptables -t nat -A PREROUTING --dst 10.11.0.1 -p udp --dport 53 -j DNAT --to-destination 10.11.0.1:5353
			iptables -t nat -A PREROUTING --dst 10.12.0.1 -p udp --dport 53 -j DNAT --to-destination 10.12.0.1:5353
			#iptables -P INPUT DROP
			iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE
			iptables -t nat -A POSTROUTING -s 10.9.0.0/24 -o eth0 -j MASQUERADE
			iptables -t nat -A POSTROUTING -s 10.10.0.0/24 -o eth0 -j MASQUERADE
			iptables -t nat -A POSTROUTING -s 10.11.0.0/24 -o eth0 -j MASQUERADE
			iptables -t nat -A POSTROUTING -s 10.12.0.0/24 -o eth0 -j MASQUERADE
			#iptables -t nat -A POSTROUTING -j MASQUERADE
			if [[ ${Linux_OS} ==  "CentOS" ]]; then
				# 相同
				service iptables save >/dev/null 2>&1
				systemctl restart iptables.service
			elif [[ ${Linux_OS} ==  "Ubuntu" ]]; then
				# 相同
				# 保存规则
				iptables-save > /FAS/iptables/fas_rules.v4
				echo 'iptables-restore < /etc/openvpn/fas_rules.v4 ' >> /root/res/auto_run
			else
				echo "程序逻辑错误，脚本已被终止..."
				exit 1;
			fi
			echo "操作已完成..."
			;;
		*)
			echo "操作已取消。"
			;;
	esac
	
	exit 0;
}


Colse_Scan()
{
	echo
	read -p "是否删除监控扫描? [Y/N] " yn
	case $yn in
		[yY]) 
			echo "正在操作中..."
			printf "%-70s"  "Stop Monitor"
			killall -9 jk.sh >/dev/null 2>&1
			echo "[ \033[32m done \033[0m ]"
			rm -rf /bin/jk.sh
			rm -rf /usr/bin/jk.sh
			echo "操作已完成...";
			;;
		*)
			echo "操作已取消。"
			;;
	esac
	exit 0;
}


Port_Tools_TCP()
{
	
	read -p "请输入TCP端口号(1-65535): " tcp_port
	while [[ ${tcp_port} == "" ]]
	do
		echo "\033[31m检测到TCP端口号没有输入，请重新尝试！\033[0m"
		read -p "请输入TCP端口号(1-65535): " tcp_port
	done
	
	
	echo "正在操作中..."
	if [ ! -f /root/res/Proxy.bin ]; then
		echo "操作失败, Proxy.bin文件不存在...";
		exit 1;
	fi
	iptables -A INPUT -p tcp -m tcp --dport $tcp_port -j ACCEPT
	/root/res/Proxy.bin -l $tcp_port -d >/dev/null 2>&1
	echo "port $tcp_port tcp" >> /root/res/portlist.conf
	
	if [[ ${Linux_OS} ==  "CentOS" ]]; then
		# 相同
		service iptables save >/dev/null 2>&1
		systemctl restart iptables.service
	elif [[ ${Linux_OS} ==  "Ubuntu" ]]; then
		# 相同
		# 保存规则
		iptables-save > /FAS/iptables/fas_rules.v4
		echo 'iptables-restore < /FAS/iptables/fas_rules.v4 ' >> /root/res/auto_run
	else
		echo "程序逻辑错误，脚本已被终止..."
		exit 1;
	fi
	
	echo "请您在服务器安全组中开放TCP $tcp_port 端口";
	echo "操作已完成...";
	exit 0;
}


Port_Tools_UDP()
{
	
	read -p "请输入UDP端口号(1-65535): " udp_port
	while [[ ${udp_port} == "" ]]
	do
		echo "\033[31m检测到UDP端口号没有输入，请重新尝试！\033[0m"
		read -p "请输入UDP端口号(1-65535): " udp_port
	done
	
	
	echo "正在操作中..."
	
	iptables -t nat -A PREROUTING -p udp --dport $udp_port -j REDIRECT --to-ports 53
	
	if [[ ${Linux_OS} ==  "CentOS" ]]; then
		# 相同
		service iptables save >/dev/null 2>&1
		systemctl restart iptables.service
	elif [[ ${Linux_OS} ==  "Ubuntu" ]]; then
		# 相同
		# 保存规则
		iptables-save > /FAS/iptables/fas_rules.v4
		echo 'iptables-restore < /FAS/iptables/fas_rules.v4 ' >> /root/res/auto_run
	else
		echo "程序逻辑错误，脚本已被终止..."
		exit 1;
	fi
	
	echo "操作已完成...";
	exit 0;
}

ADD_Port()
{
	clear
	echo "请选择端口协议类型（本程序仅适用于FAS.）"
	echo "1. TCP 转发端口(代理至Proxy)"
	echo "2. UDP 转发端口(转发至UDP53)"
	echo
	read -p "请输入选项: " Port_Tools_Option
	echo
	if [[ "$Port_Tools_Option" == "1" ]];then
		Port_Tools_TCP
		exit 0;
	fi

	if [[ "$Port_Tools_Option" == "2" ]];then
		Port_Tools_UDP
		exit 0;
	fi
		
	echo "\033[31m 输入错误！请重新运行脚本！\033[0m "
	
	exit 0;
}


Make_APP()
{
	clear
	
	echo
	echo "温馨提醒: 输入服务器IP/域名不需要带http://  但是需要填写WEB端口号"
	echo "举个栗子: 127.0.0.1:1024  端口如果是80 只需要输入服务器IP或者域名即可，不需要带80"
	read -p "请输入服务器IP/域名: " Server_IP
	while [[ ${Server_IP} == "" ]]
	do
		echo "\033[31m检测到服务器IP/域名没有输入，请重新尝试！\033[0m"
		read -p "请输入服务器IP/域名: " Server_IP
	done
	
	echo
	read -p "请设置APP名称: " APP_Name
	while [[ ${APP_Name} == "" ]]
	do
		echo "\033[31m检测到APP名称没有输入，请重新尝试！\033[0m"
		read -p "请设置APP名称: " APP_Name
	done
	
	echo
	echo "请选择下载地址"
	echo "1.Github (Global)"
	echo "2.Gitee (China)"
	read -p "请选择[1-2]: " Download_address_Option
	
	while [[ ${Download_address_Option} == "" ]]
	do
		echo "\033[31m检测到下载地址没有选择，请重新尝试！\033[0m"
		echo "请选择下载地址"
		echo "1.Github (Global)"
		echo "2.Gitee (China)"
		read -p "请选择[1-2]: " Download_address_Option
	done
	
	
	#请直接在此处修改您的下载地址
	
	if [[ ${Download_address_Option} == "1" ]];then
		echo "已选择 1.Github (Global)"
		Download_Host="https://raw.githubusercontent.com/Shirley-Jones/Fas-dingd/master/source"
	fi
	
	if [[ ${Download_address_Option} == "2" ]];then
		echo "已选择 2.Gitee (China)"
		Download_Host="https://gitee.com/JokerPan00/Fas-dingd/raw/master/resource"
	fi
	
	if [[ ${Linux_OS} ==  "CentOS" ]]; then
		# 相同
		yum install java -y >/dev/null 2>&1
	elif [[ ${Linux_OS} ==  "Ubuntu" ]]; then
		# 相同
		apt update >/dev/null 2>&1
		apt install openjdk-17-jdk -y >/dev/null 2>&1
	else
		echo "程序逻辑错误，脚本已被终止..."
		exit 1;
	fi
	
	wget --no-check-certificate -O /FAS/app/app.zip ${Download_Host}/app.zip >/dev/null 2>&1
	wget --no-check-certificate -O /FAS/app/apktool.zip ${Download_Host}/apktool.zip >/dev/null 2>&1
	unzip -o /FAS/app/app.zip -d /FAS/app/ >/dev/null 2>&1
	unzip -o /FAS/app/apktool.zip -d /FAS/app/ >/dev/null 2>&1
	java -jar /FAS/app/apktool.jar d /FAS/app/old_app.apk >/dev/null 2>&1
	java -jar /FAS/app/apktool.jar d /FAS/app/new_app.apk >/dev/null 2>&1
	sed -i 's/demo.dingd.cn:80/'${Server_IP}'/g' `grep demo.dingd.cn:80 -rl /FAS/app/old_app/smali/net/openvpn/openvpn/` 
	sed -i 's/叮咚流量卫士/'${APP_Name}'/g' "/FAS/app/old_app/res/values/strings.xml"
	sed -i 's/demo.dingd.cn:80/'${Server_IP}'/g' "/FAS/app/new_app/res/values/strings.xml"
	sed -i 's/叮咚流量卫士/'${APP_Name}'/g' "/FAS/app/new_app/res/values/strings.xml"
	java -jar /FAS/app/apktool.jar b /FAS/app/old_app >/dev/null 2>&1
	java -jar /FAS/app/apktool.jar b /FAS/app/new_app >/dev/null 2>&1
	java -jar /FAS/app/signapk.jar testkey.x509.pem testkey.pk8 /FAS/app/old_app/dist/old_app.apk /root/old_app_sign.apk >/dev/null 2>&1
	java -jar /FAS/app/signapk.jar testkey.x509.pem testkey.pk8 /FAS/app/new_app/dist/new_app.apk /root/new_app_sign.apk >/dev/null 2>&1
	rm -rf /FAS/app
	echo "生成的APP在 /root 文件夹中,请您使用Winscp等sftp协议的工具登录服务器获取APP!!!"
	echo "Android 4+ 请使用 /root/old_app_sign.apk"
	echo "Android 7+ 请使用 /root/new_app_sign.apk"
	echo "可能不兼容Android 12+以上,请自行测试..."
	echo "操作已完成..."
	exit 0;
}

# 备份原来的yum源文件
backup_yum() {
    echo "正在备份原yum源..."
    mkdir -p /etc/yum.repos.d/backup_$(date +%Y%m%d_%H%M%S)
    cp -f /etc/yum.repos.d/*.repo /etc/yum.repos.d/backup_*/ 2>/dev/null
    echo "yum源备份完成"
}

# 配置阿里云镜像源
setup_aliyun_yum() {
    echo "正在配置阿里云镜像源..."
    wget -O /etc/yum.repos.d/CentOS-Base.repo http://mirrors.aliyun.com/repo/Centos-7.repo
    if [ $? -eq 0 ]; then
        echo "阿里云镜像源配置成功"
    else
        echo "阿里云镜像源下载失败，请检查网络连接"
        exit 1
    fi
}

# 配置腾讯云镜像源  
setup_tencent_yum() {
    echo "正在配置腾讯云镜像源..."
    wget -O /etc/yum.repos.d/CentOS-Base.repo https://mirrors.cloud.tencent.com/repo/centos7_base.repo
    if [ $? -eq 0 ]; then
        echo "腾讯云镜像源配置成功"
    else
        echo "腾讯云镜像源下载失败，请检查网络连接"
        exit 1
    fi
}

# 配置华为云镜像源
setup_huawei_yum() {
    echo "正在配置华为云镜像源..."
    wget -O /etc/yum.repos.d/CentOS-Base.repo https://repo.huaweicloud.com/repository/conf/CentOS-7-reg.repo
    if [ $? -eq 0 ]; then
        echo "华为云镜像源配置成功"
    else
        echo "华为云镜像源下载失败，请检查网络连接"
        exit 1
    fi
}

# 配置网易镜像源
setup_163_yum() {
    echo "正在配置网易镜像源..."
    wget -O /etc/yum.repos.d/CentOS-Base.repo http://mirrors.163.com/.help/CentOS7-Base-163.repo
    if [ $? -eq 0 ]; then
        echo "网易镜像源配置成功"
    else
        echo "网易镜像源下载失败，请检查网络连接"
        exit 1
    fi
}

# 配置境外镜像源（vault.centos.org）
setup_vault_yum() {
    echo "正在配置境外镜像源（vault.centos.org）..."
    cat > /etc/yum.repos.d/CentOS-Base.repo << 'EOF'
#境外镜像源 - CentOS Vault
[base]
name=CentOS-$releasever - Base
baseurl=http://vault.centos.org/centos/$releasever/os/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[updates]
name=CentOS-$releasever - Updates
baseurl=http://vault.centos.org/centos/$releasever/updates/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[extras]
name=CentOS-$releasever - Extras
baseurl=http://vault.centos.org/centos/$releasever/extras/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7
EOF
    echo "境外镜像源配置完成"
}

# 清理并重建缓存
clean_cache() {
    echo "正在清理并重建yum缓存..."
    yum clean all
    yum makecache
    echo "yum缓存重建完成"
}

Repair_YUM()
{
	if [[ ${Linux_OS} ==  "CentOS" ]]; then
		# 相同
		while true; do
			clear
			echo
			echo "温馨提醒: CentOS官方已经删除了CentOS7的YUM源,此修复程序使用的2025年当前可用的其他来源的YUM源,并不保证未来的可用性."
			echo "如果未来此脚本的YUM源不可用的话,您需要考虑更换YUM源或尝试使用其他OpenVPN项目!!!"
			echo "========================================"
			echo "    CentOS 7 YUM源切换脚本 2025.12.07   "
			echo "========================================"
			echo "请选择要使用的YUM镜像源："
			echo "1) 阿里云镜像源（国内推荐）"
			echo "2) 腾讯云镜像源"
			echo "3) 华为云镜像源"
			echo "4) 网易163镜像源"
			echo "5) 境外镜像源（vault.centos.org）"
			echo "6) 退出"
			echo "========================================"
			read -p "请输入选择序号 (1-6): " choice
			
			case $choice in
				1)
					backup_yum
					setup_aliyun_yum
					clean_cache
					;;
				2)
					backup_yum
					setup_tencent_yum
					clean_cache
					;;
				3)
					backup_yum
					setup_huawei_yum
					clean_cache
					;;
				4)
					backup_yum
					setup_163_yum
					clean_cache
					;;
				5)
					backup_yum
					setup_vault_yum
					clean_cache
					;;
				6)
					echo "退出脚本"
					exit 0
					;;
				*)
					echo "无效选择，请重新输入"
					sleep 2
					;;
			esac
		done
	else
		echo "此功能只能CentOS 7系统使用!!!"
		exit 1;
	fi
}


System_Check()
{
	# 检查root权限
    if [ $EUID -ne 0 ]; then
        echo "错误：请使用root权限运行此脚本"
        exit 1
    fi
	
	# 检查wget是否安装
	if ! command -v wget &> /dev/null; then
		echo "错误：wget未安装，请先安装wget"
		echo "CentOS系统: yum install wget -y"
		echo "Ubuntu系统: apt install wget -y"
		exit 1
	fi

	# 检查wget是否安装
	if ! command -v curl &> /dev/null; then
		echo "错误：curl未安装，请先安装curl"
		echo "CentOS系统: yum install curl -y"
		echo "Ubuntu系统: apt install curl -y"
		exit 1
	fi
	
	if grep -Eqii "CentOS" /etc/issue || grep -Eq "CentOS" /etc/*-release; then
		Linux_OS='CentOS'
		PM='yum'
	elif grep -Eqi "Red Hat Enterprise Linux Server" /etc/issue || grep -Eq "Red Hat Enterprise Linux Server" /etc/*-release; then
		Linux_OS='RHEL'
		PM='yum'
	elif grep -Eqi "Aliyun" /etc/issue || grep -Eq "Aliyun" /etc/*-release; then
		Linux_OS='Aliyun'
		PM='yum'
	elif grep -Eqi "Fedora" /etc/issue || grep -Eq "Fedora" /etc/*-release; then
		Linux_OS='Fedora'
		PM='yum'
	elif grep -Eqi "Debian" /etc/issue || grep -Eq "Debian" /etc/*-release; then
		Linux_OS='Debian'
		PM='apt'
	elif grep -Eqi "Ubuntu" /etc/issue || grep -Eq "Ubuntu" /etc/*-release; then
		Linux_OS='Ubuntu'
		PM='apt'
	elif grep -Eqi "Raspbian" /etc/issue || grep -Eq "Raspbian" /etc/*-release; then
		Linux_OS='Raspbian'
		PM='apt'
	else
		Linux_OS='Unknown'
	fi
	
	source /etc/os-release
	Linux_Version=${VERSION_ID}
	
	if [[ ${Linux_OS} ==  "CentOS" ]]; then
		# 支持的
		if [[ !${Linux_Version} ==  "7" ]]; then 
			echo "当前Linux系统不支持安装FAS流控,请更换系统后重新尝试."
			exit 1;
		fi
	elif [[ ${Linux_OS} ==  "Ubuntu" ]]; then
		# 支持的
		result=$(echo "$Linux_Version < 20.04" | bc -l)
		if [ $result -eq 1 ]; then
			echo "当前Linux系统不支持安装FAS流控,请更换系统后重新尝试."
			exit 1;
		fi
	else
		echo "当前的Linux系统不支持安装FAS流控,请更换系统后重新尝试."
		exit 1;
	fi
}

Main()
{
	rm -rf $0 >/dev/null 2>&1
	System_Check
	clear
	while true; do
        clear
        echo "********************************************************"
		echo "              欢迎使用FAS系统快速安装助手               "
		echo "  项目地址: https://github.com/Shirley-Jones/Fas-dingd  "
		echo "        流控版权为筑梦网络科技(筑梦工作室)所有          "
		echo "  此脚本由Shirley开源，与筑梦网络科技(筑梦工作室)无关！ "
		echo "********************************************************"
		echo 
        echo "1. 安装FAS3.0系统  ♪～(´ε｀) "
        echo "2. FAS系统负载(多台服务器集群负载) "
        echo "3. 重置防火墙 (解决冬云等服务器安装没网) "
        echo "4. 删除负载机监控的扫描 "
        echo "5. 添加TCP/UDP端口 "
		echo "6. 生成APP "
		echo "7. 修复YUM源 "
        echo "0. 退出程序 "
        echo
        
        read -p "请输入选项: " Install_Option
        
        case "$Install_Option" in
            "1") Install_FAS ;;
            "2") Cluster_load ;;
            "3") Reset_iptables ;;
            "4") Colse_Scan ;;
            "5") ADD_Port ;;
            "6") Make_APP ;;
			"7") Repair_YUM ;;
            "0") 
                echo "程序已退出"
                exit 0
                ;;
            *) 
                echo "输入错误！请重新选择！"
                read -p "按回车键继续..."
                ;;
        esac
    done
}

Main
exit 0;

