#include "BfxBotConfig.h"

BfxBotConfig::BfxBotConfig()
{
	apiKey = "";
	secretKey = "";
}

BfxBotConfig::~BfxBotConfig()
{
	return;//do nothing
}

int BfxBotConfig::Init(char* pathToConfig)
{
	int cc = SUCCESS;
	fstream fileHandle;
	fileHandle.open(pathToConfig, std::fstream::in);
	if (fileHandle.is_open())
	{

		fileHandle.seekg(0, fileHandle.end);
		unsigned long  fileLength = fileHandle.tellg();
		char* buffer = new char[fileLength];
		memset(buffer, 0, fileLength);
		fileHandle.seekg(0, fileHandle.beg);

		fileHandle.read(buffer, fileLength);

		if (doc.IsObject())
			doc.GetAllocator().Clear();
		ParseResult parseResult = doc.Parse(buffer, fileLength);

		if (parseResult.IsError() == false)
		{
			cc = ExtractConfigData();
		}
		else
			cc = INVALID_CONFIG_DATA;

		delete[] buffer;
		fileHandle.close();
	}
	else
		cc = CONFIG_NOT_FOUND;

	return cc;
}

int BfxBotConfig::ExtractConfigData()
{
	int cc = SUCCESS;

	do
	{

		if (cc == SUCCESS && doc.HasMember("Log Path") && doc["Log Path"].IsString())
		{
			logPath = doc["Log Path"].GetString();
			Logger::getInstance(&logPath)->setLevel(Logger::LEVEL_TRACE);//init logger here
		}
		else
		{
			LOG_ERROR("Invalid Log Path Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}
		
		if (cc == SUCCESS && doc.HasMember("Api-Key") && doc["Api-Key"].IsString())
		{
			apiKey = doc["Api-Key"].GetString();
		}
		else
		{
			LOG_ERROR("Invalid Api-Key Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		if (cc == SUCCESS && doc.HasMember("Secret-Key") && doc["Secret-Key"].IsString())
		{
			secretKey = doc["Secret-Key"].GetString();
		}
		else
		{
			LOG_ERROR("Invalid Secret-Key Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		if (cc == SUCCESS && doc.HasMember("Send To Email") && doc["Send To Email"].IsString())
		{
			toEmail = doc["Send To Email"].GetString();
		}
		else
		{
			LOG_ERROR("Invalid Email Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		if (cc == SUCCESS && doc.HasMember("Email User") && doc["Email User"].IsString())
		{
			emailUser = doc["Email User"].GetString();
		}
		else
		{
			LOG_ERROR("Invalid Email User Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		if (cc == SUCCESS && doc.HasMember("Email Pass") && doc["Email Pass"].IsString())
		{
			emailPass = doc["Email Pass"].GetString();
		}
		else
		{
			LOG_ERROR("Invalid Email Pass Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		

		if (cc == SUCCESS && doc.HasMember("Symbol") && doc["Symbol"].IsString())
		{
			symbol = doc["Symbol"].GetString();
		}
		else
		{
			LOG_ERROR("Invalid Symbol Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		if (cc == SUCCESS && doc.HasMember("Cooldown Timer") && doc["Cooldown Timer"].IsInt())
		{
			cooldownTimer = doc["Cooldown Timer"].GetInt();//in milliseconds
		}
		else
		{
			LOG_ERROR("Invalid Cooldown Timer Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		if (cc == SUCCESS && doc.HasMember("Percent Margin") && doc["Percent Margin"].IsInt())
		{
			percentMarginToUse = doc["Percent Margin"].GetInt();
			if (percentMarginToUse > 330 && percentMarginToUse <= 0)//330 is max margin on bfx
			{
				LOG_ERROR("percentMarginToUse must be equal to or less or equal to than 330 and greater than 0");
				cc = INVALID_CONFIG_VALUE;
				break;
			}
		}
		else
		{
			LOG_ERROR("Invalid percentMarginToUse Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		if (cc == SUCCESS && doc.HasMember("Persisent Data Path") && doc["Persisent Data Path"].IsString())
		{
			persistentDataPath = doc["Persisent Data Path"].GetString();
		}
		else
		{
			LOG_ERROR("Invalid Persisent Data Path Value");
			cc = INVALID_CONFIG_VALUE;
			break;
		}

		if (cc == SUCCESS && doc.HasMember("Candle Time Frame") && doc["Candle Time Frame"].IsString())
		{
			std::string localTimeFrameStr = doc["Candle Time Frame"].GetString();

			if (localTimeFrameStr.compare(FIVE_M_TIMEFRAME) == SUCCESS)
				candleTimeFrame = five_minute;
			else if(localTimeFrameStr.compare(FIFTEEN_M_TIMEFRAME) == SUCCESS)
				candleTimeFrame = fifteen_minute;
			else if (localTimeFrameStr.compare(THIRTY_M_TIMEFRAME) == SUCCESS)
				candleTimeFrame = thirty_minute;
			else if (localTimeFrameStr.compare(ONE_H_TIMEFRAME) == SUCCESS)
				candleTimeFrame = one_hour;
			else if (localTimeFrameStr.compare(THREE_H_TIMEFRAME) == SUCCESS)
				candleTimeFrame = three_hour;
			else if (localTimeFrameStr.compare(SIX_H_TIMEFRAME) == SUCCESS)
				candleTimeFrame = six_hour;
			else if (localTimeFrameStr.compare(TWELVE_H_TIMEFRAME) == SUCCESS)
				candleTimeFrame = twelve_hour;
			else if (localTimeFrameStr.compare(ONE_D_TIMEFRAME) == SUCCESS)
				candleTimeFrame = one_day;
			else if (localTimeFrameStr.compare(ONE_W_TIMEFRAME) == SUCCESS)
				candleTimeFrame = one_week;
			else
			{
				LOG_ERROR("Invalid Candle Time Frame, must be 5m,15m,30m,1h,3hr,6hr,12hr,1D,1w");
				cc = INVALID_CONFIG_VALUE;
				break;
			}
			
		}
		else
		{
			LOG_ERROR("Invalid Candle Time Frame Data");
			cc = INVALID_CONFIG_VALUE;
			break;
		}


	} while (false);
	

	return cc;
}



void BfxBotConfig::GetApiKey(std::string* output)
{
	*output = apiKey;
	return;
}

void BfxBotConfig::GetSecretKey(std::string* output)
{
	*output = secretKey;
	return;
}

void BfxBotConfig::GetSendToEmail(std::string * output)
{
	*output = toEmail;
	return;
}

void BfxBotConfig::GetEmailUser(std::string * output)
{
	*output = emailUser;
	return;
}

void BfxBotConfig::GetEmailPass(std::string * output)
{
	*output = emailPass;
	return;
}

void BfxBotConfig::GetLogPath(std::string * output)
{
	*output = logPath;
	return;
}

void BfxBotConfig::GetSymbol(std::string * output)
{
	*output = symbol;
	return;
}

int BfxBotConfig::GetCoolDownTimer()
{
	return cooldownTimer;
}

int BfxBotConfig::GetPercentMarginToUse()
{
	return percentMarginToUse;
}

BfxBotConfig::CandleTimeFrame BfxBotConfig::GetCandleTimeFrame()
{
	return candleTimeFrame;
}

void BfxBotConfig::GetPersistentDataPath(std::string * output)
{
	*output = persistentDataPath;
	return;
}

