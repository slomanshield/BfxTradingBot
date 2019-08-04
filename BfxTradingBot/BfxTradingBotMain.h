#pragma once
#include "global.h"
#include "BfxLibrary.h"
#include "BfxBotConfig.h"
#include "ThreadWrapper.h"

#define ReconnectTimeoutSecs 10
#define NO_ACUTAL_POSITION_TIMEOUT 5
#define MARGIN_MAX 3.335
#define DRIFT_OFFSET 5


typedef void BotMainThread(bool* running, bool* stopped, void* usrPtr);

class BfxTradingBotMain
{
	enum {get_tradeable_balance,create_order,close_position,take_profit};
	enum MarketPosition{not_calculated,above_pp,below_pp};
	public:
		BfxTradingBotMain();
		~BfxTradingBotMain();
		int Init(char* pathToConfig);
		int Process();
		int SendEmail(std::string to, std::string to_addr, std::string message, std::string subject);
		int SendCurrentErrorFile();
		void GetLogPath(std::string* out);
		bool Start();
		bool End();
		int ReConnectWebSocketTradingChannel();
		int ReConnectWebSocketAuth();
	private:
		std::string symbol;
		std::string apiKey;
		std::string secretKey;
		std::string toEmail;
		std::string emailUser;
		std::string emailPass;
		std::string logPath;
		std::string persisantDataPath;
		BfxBotConfig::CandleTimeFrame candletimeFrame;
		int cooldownTimer;
		int percentMarginToUse;
		double lastCurrentPrice;
		double currentPrice;
		double pivotPoint;
		double s1Short;
		double s2Short;
		double s3Short;
		double r1Long;
		double r2Long;
		double r3Long;
		double minimumOrderAmount;
		double positionAmount;
		double marginBalance;
		double marginBalance_PL;
		double percentMarginToUseDecimal;

		time_t lastTimeMinimumOrder;
		time_t futurePivotTime;
		time_t lastPivotTime;
		time_t lastAmountNonZero;
		HighResolutionTimePoint lastFlipExecuted;

		bool ErrorConnectingBfx;
		bool needPivotData;
		bool firsTimePivotData;
		MarketPosition marketPosition;

		/* PERSISTENT DATA DO NOT MOVE*/
		bool s1TakeProfit;
		bool s2TakeProfit;
		bool s3TakeProfit;
		bool r1TakeProfit;
		bool r2TakeProfit;
		bool r3TakeProfit;
		/* PERSISTENT DATA DO NOT MOVE always append to the end*/
		int persistentDataLength = sizeof(bool) * 6;
		char* persistantDataStartPtr = (char*)&s1TakeProfit;

		std::string GenerateEmailMessageId();
		std::string GenerateEmailPayload(std::string to, std::string to_addr, std::string message, std::string subject);
		int CalculateWeeklyPivot();
		int CalculateTimeFramePivot();
		int GetMinimuimOrderAmount();
		void ResetTakeProfit();
		int ReadPersistentData();
		int WritePersistentdata();

		BfxBotConfig bfxBotConfig;
		BfxLibrary   bfxLibrary;
		time_t lastTryReconnect;


		bool IsTimeToReconnect();
		bool IsTimeToCalcWeeklyPivot();
		bool IsTimeToCalcTimeFramePivot();
		bool IsTimeToGetMinimumOrderAmount();
		bool IsAcutallyEmpty();
		bool IsOkToFlip();

		int    GetCreateOrderSize(double* orderSize_out);//if the size is more than the tradeable balance_PL log an error, if less that minimum do minimum
		int    TakeProfit(double percentDecimal);//percent in decminal,if position size - (position size * %) is less than minimum just return
		int    ClosePosition();//jsut close the position
		int    CreateOrder(double amount, bool long_order);//open a position long or short


		int ProcessWrapper(int type, int num_args,...);

		ThreadWrapper<BotMainThread> wrapper;
		static BotMainThread ProcessTrades;

		
};


class stringdata {
public:
	std::string msg;
	size_t bytesleft;

	stringdata(std::string &&m)
		: msg{ m }, bytesleft{ msg.size() }
	{}
	stringdata(std::string &m) = delete;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	stringdata *text = reinterpret_cast<stringdata *>(userp);

	if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1) || (text->bytesleft == 0)) {
		return 0;
	}

	if ((nmemb * size) >= text->msg.size()) {
		text->bytesleft = 0;
		return text->msg.copy(reinterpret_cast<char *>(ptr), text->msg.size());
	}

	return 0;
}