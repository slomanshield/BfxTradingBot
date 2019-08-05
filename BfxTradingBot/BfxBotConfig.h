#pragma once
#include "global.h"

#define FIVE_M_TIMEFRAME "5m"
#define FIFTEEN_M_TIMEFRAME "15m"
#define THIRTY_M_TIMEFRAME "30m"
#define ONE_H_TIMEFRAME "1h"
#define THREE_H_TIMEFRAME "3h"
#define SIX_H_TIMEFRAME "6h"
#define TWELVE_H_TIMEFRAME "12h"
#define ONE_D_TIMEFRAME "1D"
#define ONE_W_TIMEFRAME "1w"

#define NUM_TIMEFRAMES 9

static char * time_frame_array[NUM_TIMEFRAMES] =
{ "5m",
  "15m",
  "30m",
  "1h",
  "3h",
  "6h",
  "12h",
  "1D",
  "1w" };

class BfxBotConfig
{
	
	public:
		enum CandleTimeFrame { five_minute, fifteen_minute, thirty_minute, one_hour, three_hour, six_hour, twelve_hour, one_day, one_week };
		BfxBotConfig();
		~BfxBotConfig();
		int Init(char* pathToConfig);
		int ExtractConfigData();
		void GetApiKey(std::string* output);
		void GetSecretKey(std::string* output);
		void GetSendToEmail(std::string* output);
		void GetEmailUser(std::string* output);
		void GetEmailPass(std::string* output);
		void GetLogPath(std::string* output);
		void GetSymbol(std::string* output);
		void GetPersistentDataPath(std::string* output);
		int  GetCoolDownTimer();
		int  GetPercentMarginToUse();
		CandleTimeFrame GetCandleTimeFrame();
	private:
		std::string apiKey;
		std::string secretKey;
		std::string toEmail;
		std::string emailUser;
		std::string emailPass;
		std::string logPath;
		std::string symbol;//XXXYYY format
		std::string persistentDataPath;
		CandleTimeFrame candleTimeFrame;
		int cooldownTimer;
		int percentMarginToUse;//whole number up to 330
		Document doc;
};

typedef BfxBotConfig::CandleTimeFrame ConfigTimeFrame;