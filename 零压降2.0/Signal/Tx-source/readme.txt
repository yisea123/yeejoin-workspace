ng版本基于基础版本的说明：
 ** 添加了对信号的正半周幅值进行调整
 ** 调零机制优化
 ** 设置命令的规整
 ** 对ADC采样值的微分积分误差进行补偿
 ** 增加读温度、软件版本的命令
 ** 系统维护串口采用中断的方式进行数据的发送和接收

================================================================================================================

-----------------------------------------------
TX-NG-0.12修复bug：
1、写flash时概率性的被狗咬死

-----------------------------------------------
TX-NG-0.11改进：
1、修改默认参数值

-----------------------------------------------
TX-NG-0.10改进：
1、采集端信号灯是否闪烁的判断

-----------------------------------------------
TX-NG-0.09修复bug：
1、前300个点的值不对

-----------------------------------------------
TX-NG-0.08添加新功能：
1、启用独立看门狗和窗口看门狗
2、添加查看系统运行时间的命令


-----------------------------------------------
TX-NG-0.07修复bug：
1、配置完成后，再开启systick，编写初始化阶段将参数到flash中的函数。


-----------------------------------------------
TX-NG-0.06修复bug：
1、应该先初始化串口缓冲区

-----------------------------------------------
TX-NG-0.05修复bug：
1、解决竞争条件引起的零位参数错误问题

-----------------------------------------------
TX-NG-0.04改进：
1、增加读配置参数的命令
2、调零位命令的优化

-----------------------------------------------
TX-NG-0.03改进：
1、优化调试接口的命令处理
2、串口的接收引脚设置为内部上拉（悬浮输入，容易受到干扰）
3、将幅值、相位、零位、对称度的参数值都设置为经验值

-----------------------------------------------
TX-NG-0.02修复bug：
1、保存15分钟时的温度控制参数值，导致上电后，10多分钟的加热，延迟读signal_cfg_param.temper_pwm可以读取正确数据。

-----------------------------------------------
TX-NG-0.01新增功能：
//1、增加与信号恢复端的同步机制（因此，该版本不能与恢复端的老版本软件配套使用） ==== 接收端使用6字节dma时，波形不好，所以取消
1、添加了保存温度pwm的功能（保存上电15分钟后的值，将写flash操作移到后台，发送数据的移到前台）
2、添加进入、退出参数设定模式的命令

-----------------------------------------------
TX-NG-0.01改进：
1、dma1_1, dma1_7的中断优先级需要相同。
2、整理stm32f10x_it.c文件中的函数。
3、采集端的信号灯闪烁表示信号正常，有一相缺相时，led灯熄灭。
4、usart1接收到3字节处理一次，处理后清楚接收缓冲区，这样可以避免一次接收错误后，以后都不能正常处理
5、零位调整的参数默认值改为0x8000




================================================================================================================
================================================================================================================
================================================================================================================
================================================================================================================

7.7_采45us-15000-零位OK-调整到364-放狗

程序更新说明：
/!\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\!/
1.	[2010.12.06] Ver2.0.4
			当前是使用的DMA非连续模式
			定时器触发TIM3 2us 
			在DMA中断中关闭 TIM3 用Sysstick定时20us启动TIM3 （包括配置 和使能TIM3）
			ADC采样 只是采样一次 外推后直接将数据发送到USART1 BaudRate = 3M
			USART2 BaudRate = 115200 AA +(外推时间两个字节 )[现在是100ns作为步进]


2.	[2011.01.03] Ver1.0.1
			当前是使用的DMA  扫描非模式 每次在sys定时中重新配置TIM3和DMA
			三相ID采用的最高的四位热键码 数据是每相一个ID号


3.	[2011.01.15] Ver2.0.1
			当前ADC是使用的DMA  扫描模式  每次在sys定时中重新配置DMA 和软件启动			ADC。
			UART采用DMA完成传输，关闭UART，重新配置DMA。 
			
			三相ID采用的最高的两位1，2，3 ，将数据分为6+6 = 12
			高 6bit 上的最高两位采用1 2 3分别标识，低6bit高两个Bit为00



/!\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\!/
程序更新心得：
1.	
	[2010.12.06] Ver2.0.4 	定时器TIM3 触发源  触发时间为2us
				不能太短，否则会有问题，但问题不知道为何，
				具体有时间在探讨。
	