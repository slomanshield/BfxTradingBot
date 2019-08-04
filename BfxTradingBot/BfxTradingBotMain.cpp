#include "BfxTradingBotMain.h"

BfxTradingBotMain::BfxTradingBotMain()
{
	symbol = "";
	apiKey = "";
	secretKey = "";
	emailUser = "";
	emailPass = "";
	logPath = "";
	cooldownTimer = -1;
	percentMarginToUse = 0;
	lastCurrentPrice = -1;
	currentPrice = -1;
	needPivotData = true;
	futurePivotTime = 0;
	lastTimeMinimumOrder = 0;
	minimumOrderAmount = -1;
	marginBalance = -1;
	marketPosition = MarketPosition::not_calculated;
	lastAmountNonZero = 0;
	firsTimePivotData = true;
}

BfxTradingBotMain::~BfxTradingBotMain()
{

}

int BfxTradingBotMain::Init(char * pathToConfig)
{
	int cc = SUCCESS;

	cc = bfxBotConfig.Init(pathToConfig);
	wrapper.SetProcessthread(BfxTradingBotMain::ProcessTrades);
	if (cc == SUCCESS)
	{
		bfxBotConfig.GetEmailUser(&emailUser);
		bfxBotConfig.GetEmailPass(&emailPass);
		bfxBotConfig.GetSymbol(&symbol);
		cooldownTimer = bfxBotConfig.GetCoolDownTimer();
		percentMarginToUse = bfxBotConfig.GetPercentMarginToUse();
		bfxBotConfig.GetLogPath(&logPath);
		bfxBotConfig.GetPersistentDataPath(&persisantDataPath);
		candletimeFrame = bfxBotConfig.GetCandleTimeFrame();

		percentMarginToUseDecimal = percentMarginToUse / 100.00;
	}

	if (cc == SUCCESS)
	{
		bfxBotConfig.GetApiKey(&apiKey);
		bfxBotConfig.GetSecretKey(&secretKey);
		bfxLibrary.InitKeys((char*)apiKey.c_str(), (char*)secretKey.c_str());
	}
	ResetTakeProfit();//reset then read in
	Start();

	ReadPersistentData();

	return cc;
}

int BfxTradingBotMain::SendEmail(std::string to, std::string to_addr, std::string message, std::string subject)
{
	int cc = SUCCESS;
	stringdata payload = GenerateEmailPayload(to, to_addr, message, subject);
	CURL *curl;
	struct curl_slist *recipients = NULL;
	CURLcode res = CURLE_OK;
	curl = curl_easy_init();

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, GOOGLE_SMTP);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		curl_easy_setopt(curl, CURLOPT_PORT, 465);
		curl_easy_setopt(curl, CURLOPT_USERNAME, emailUser.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, emailPass.c_str());

		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM_EMAIL);

		recipients = curl_slist_append(recipients, to_addr.c_str());
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &payload);

		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			LOG_ERROR(string_format(std::string("Error sending email: %s"), curl_easy_strerror(res)));
			cc = ErrorSendingEamil;
		}

		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
	}


	return cc;
}

int BfxTradingBotMain::SendCurrentErrorFile()
{
	int cc = SUCCESS;
	std::string pathToLog;
	fstream fileHandle;
	time_t now = time(0);
	struct tm* ptm = localtime(&now);
	char buf[] = "YYYYMMDD.log";

	snprintf(buf, sizeof(buf), "%04d%02d%02d.log", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
	bfxBotConfig.GetSendToEmail(&toEmail);

	fileHandle.open(std::string(logPath + "/" + buf).c_str(), std::fstream::in);
	if (fileHandle.is_open())
	{
		fileHandle.seekg(0, fileHandle.end);
		unsigned long  fileLength = fileHandle.tellg();
		if (fileLength > 0)
		{
			char* buffer = new char[fileLength];
			memset(buffer, 0, fileLength);
			fileHandle.seekg(0, fileHandle.beg);

			fileHandle.read(buffer, fileLength);

			cc = SendEmail("User", toEmail, std::string(buffer, fileLength), std::string("ERROR LOG"));

			delete[] buffer;
		}

		fileHandle.close();
	}

	return cc;
}

void BfxTradingBotMain::GetLogPath(std::string * out)
{
	*out = logPath;
}

bool BfxTradingBotMain::Start()
{
	return wrapper.StartProcessing(1, this);//start 1 thread and pass in object pointer
}

bool BfxTradingBotMain::End()
{
	return wrapper.StopProcessing(10000);//wait for 10 secs
}

std::string BfxTradingBotMain::GenerateEmailMessageId()
{
	const int MESSAGE_ID_LEN = 37;
	srand(GET_NONCE);

	time_t t;


	std::string ret;
	ret.resize(15);

	time(&t);
	struct tm* tm = gmtime(&t);

	strftime(const_cast<char *>(ret.c_str()),
		MESSAGE_ID_LEN,
		"%Y%m%d%H%M%S.",
		tm);

	ret.reserve(MESSAGE_ID_LEN);

	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	while (ret.size() < MESSAGE_ID_LEN) {
		ret += alphanum[rand() % (sizeof(alphanum) - 1)];
	}


	return ret;
}

std::string BfxTradingBotMain::GenerateEmailPayload(std::string to, std::string to_addr, std::string message, std::string subject)
{
	std::string payloadTemplate = "Date: %s \r\n"
		"To: %s <%s> \r\n"
		"From: %s <%s> \r\n"
		"Cc: <> \r\n"
		"Message-ID: <%s@%s> \r\n"
		"Subject: %s \r\n"
		"\r\n"
		"%s \r\n"
		"\r\n"
		"\r\n"
		"\r\n";

	std::string fromEmail = FROM_EMAIL;

	std::string payload = string_format(payloadTemplate, dateTimeNow().c_str(), to.c_str(), to_addr.c_str(), FROM_NAME, fromEmail.c_str(), GenerateEmailMessageId().c_str(),
		fromEmail.substr(fromEmail.find('@') + 1).c_str(), subject.c_str(), message.c_str());

	return payload;
}

void BfxTradingBotMain::ProcessTrades(bool* running, bool* stopped, void* usrPtr)
{
	BfxTradingBotMain* bfxBotMain = (BfxTradingBotMain*)usrPtr;
	while (*running == true)
	{
		bfxBotMain->currentPrice = bfxBotMain->bfxLibrary.GetCurrentPrice();
		if (bfxBotMain->bfxLibrary.GetWebSocketTradeChannelStatus() == ReadyState::Open && bfxBotMain->currentPrice != -1 && bfxBotMain->needPivotData != true
			&& bfxBotMain->minimumOrderAmount != -1)//if we are connected to the trade channel,have our pivot data, and have a minimum order amount
		{
			if (bfxBotMain->lastCurrentPrice != bfxBotMain->currentPrice)
			{

				if (bfxBotMain->marketPosition == MarketPosition::not_calculated)
				{
					if (bfxBotMain->currentPrice > bfxBotMain->pivotPoint)
						bfxBotMain->marketPosition = MarketPosition::above_pp;
					else if(bfxBotMain->currentPrice < bfxBotMain->pivotPoint)
						bfxBotMain->marketPosition = MarketPosition::below_pp;
				}

				if (bfxBotMain->bfxLibrary.GetCurrentPositionAmount() != 0)//we are in an open position... what to do
				{
					if (bfxBotMain->currentPrice > bfxBotMain->pivotPoint)
					{
						if (bfxBotMain->bfxLibrary.GetCurrentPositionAmount() > 0)//this is good long is in profit, lets try and take some profit
						{
							if (bfxBotMain->currentPrice > bfxBotMain->r1Long && bfxBotMain->r1TakeProfit == false)//take profit if we havent already
							{
								bfxBotMain->TakeProfit(.5);
								bfxBotMain->r1TakeProfit = true;
								bfxBotMain->WritePersistentdata();
								LOG_TRACE(string_format("Taking r1Long Profit... At : %lf", bfxBotMain->currentPrice).c_str());
							}
							else if (bfxBotMain->currentPrice > bfxBotMain->r2Long && bfxBotMain->r2TakeProfit == false)
							{
								bfxBotMain->TakeProfit(.5);
								bfxBotMain->r2TakeProfit = true;
								bfxBotMain->WritePersistentdata();
								LOG_TRACE(string_format("Taking r2Long Profit... At : %lf",bfxBotMain->currentPrice).c_str());
							}
							else if (bfxBotMain->currentPrice > bfxBotMain->r3Long && bfxBotMain->r3TakeProfit == false)
							{
								bfxBotMain->TakeProfit(.5);
								bfxBotMain->r3TakeProfit = true;
								bfxBotMain->WritePersistentdata();
								LOG_TRACE(string_format("Taking r3Long Profit... At : %lf",bfxBotMain->currentPrice).c_str());
							}
						}
						else if (bfxBotMain->bfxLibrary.GetCurrentPositionAmount() < 0)// this is bad short is underwater
						{
							if (bfxBotMain->IsOkToFlip())//dont want to constantly flip, set cool down in config to 0 to always return true
							{
								bfxBotMain->ClosePosition();
								bfxBotMain->CreateOrder(0, true);//calculate based on percent config 
								bfxBotMain->ResetTakeProfit();
								bfxBotMain->WritePersistentdata();
								LOG_TRACE(string_format("Flipping from short to long... At : %lf", bfxBotMain->currentPrice).c_str());
							}
							
						}

						bfxBotMain->marketPosition = MarketPosition::above_pp;
					}
					else if (bfxBotMain->currentPrice < bfxBotMain->pivotPoint)
					{
						if (bfxBotMain->bfxLibrary.GetCurrentPositionAmount() < 0)//this is good short is in profit
						{
							if (bfxBotMain->currentPrice < bfxBotMain->s1Short && bfxBotMain->s1TakeProfit == false)//take profit if we havent already
							{
								bfxBotMain->TakeProfit(.5);
								bfxBotMain->s1TakeProfit = true;
								bfxBotMain->WritePersistentdata();
								LOG_TRACE(string_format("Taking s1Short Profit... At : %lf", bfxBotMain->currentPrice).c_str());
							}
							else if (bfxBotMain->currentPrice < bfxBotMain->s2Short && bfxBotMain->s2TakeProfit == false)
							{
								bfxBotMain->TakeProfit(.5);
								bfxBotMain->s2TakeProfit = true;
								bfxBotMain->WritePersistentdata();
								LOG_TRACE(string_format("Taking s2Short Profit... At : %lf", bfxBotMain->currentPrice).c_str());
							}
							else if (bfxBotMain->currentPrice < bfxBotMain->s3Short && bfxBotMain->s3TakeProfit == false)
							{
								bfxBotMain->TakeProfit(.5);
								bfxBotMain->s3TakeProfit = true;
								bfxBotMain->WritePersistentdata();
								LOG_TRACE(string_format("Taking s3Short Profit... At : %lf", bfxBotMain->currentPrice).c_str());
							}
						}
						else if (bfxBotMain->bfxLibrary.GetCurrentPositionAmount() > 0) //this is bad long is underwater
						{
							if (bfxBotMain->IsOkToFlip())//dont want to constantly flip, set cool down in config to 0 to always return true
							{
								bfxBotMain->ClosePosition();
								bfxBotMain->CreateOrder(0, false);//calculate based on percent config 
								bfxBotMain->ResetTakeProfit();
								bfxBotMain->WritePersistentdata();
								LOG_TRACE(string_format("Flipping from long to short... At : %lf", bfxBotMain->currentPrice).c_str());
							}
						}

						bfxBotMain->marketPosition = MarketPosition::below_pp;
					}
					time(&bfxBotMain->lastAmountNonZero);
				}
				else if(bfxBotMain->IsAcutallyEmpty()) //I guess we have no open position lets try and wait for a cross then open one
				{
					if (bfxBotMain->currentPrice > bfxBotMain->pivotPoint && bfxBotMain->marketPosition == MarketPosition::below_pp)
					{
						bfxBotMain->marketPosition = MarketPosition::above_pp;
						bfxBotMain->CreateOrder(0, true);//calculate based on percent config 
						LOG_TRACE(string_format("Opening a long... At : %lf", bfxBotMain->currentPrice));
					}
					else if (bfxBotMain->currentPrice < bfxBotMain->pivotPoint && bfxBotMain->marketPosition == MarketPosition::above_pp)
					{
						bfxBotMain->marketPosition = MarketPosition::below_pp;
						bfxBotMain->CreateOrder(0, false);//calculate based on percent config 
						LOG_TRACE(string_format("Opening a short... At : %lf", bfxBotMain->currentPrice).c_str());
					}
					//else lmao i guess we are at the pivot point, looks like we are in really choopy waters
				}
				
				bfxBotMain->lastCurrentPrice = bfxBotMain->currentPrice;
			}
		}
		usleep(10000);//sleep for 10 milliseconds
	}
	*stopped = true;
}

int BfxTradingBotMain::ReConnectWebSocketTradingChannel()
{
	int cc = SUCCESS;
	bfxLibrary.DisconnetWebSocketTradeChannel();
	cc = bfxLibrary.ConnectWebSocketTradeChannel();

	if (cc == SUCCESS)
	{
		cc = bfxLibrary.SubscribeToTradeChannel(&symbol);
	}
	else
		LOG_ERROR_CODE(cc);

	return cc;
}

int BfxTradingBotMain::ReConnectWebSocketAuth()
{
	int cc = SUCCESS;
	bfxLibrary.DisconnetWebSocketAuth();
	cc = bfxLibrary.ConnectWebSocketAuth();

	if (cc == SUCCESS)
	{
		cc = bfxLibrary.SendAuthWebSocket(&symbol);
	}
	else
		LOG_ERROR_CODE(cc);

	return cc;
}

bool BfxTradingBotMain::IsTimeToReconnect()
{
	time_t currTime = time(NULL);
	if (currTime - lastTryReconnect >= ReconnectTimeoutSecs)
	{
		lastTryReconnect = currTime;
		return true;
	}
	return false;
}

bool BfxTradingBotMain::IsTimeToGetMinimumOrderAmount()
{
	time_t currTime = time(NULL);
	if (currTime - lastTimeMinimumOrder >= DAILY_SECS)
	{
		lastTimeMinimumOrder = currTime;
		return true;
	}
	return false;
}

bool BfxTradingBotMain::IsAcutallyEmpty()
{
	time_t currTime = time(NULL);
	if (currTime - lastAmountNonZero >= NO_ACUTAL_POSITION_TIMEOUT)
	{
		lastAmountNonZero = currTime;//set a 5 sec cooldown for the position to reach us
		return true;
	}
	return false;
}

bool BfxTradingBotMain::IsOkToFlip()
{
	HighResolutionTimePoint currTime = HighResolutionTime::now();
	DoubleMili difftime;
	if (cooldownTimer == 0)
		return true;//if 0 just say yes we always flipping

	difftime = currTime - lastFlipExecuted;
	if (difftime.count() > cooldownTimer)
	{
		lastFlipExecuted = currTime;
		return true;
	}
	return false;
}

int BfxTradingBotMain::GetCreateOrderSize(double* orderSize_out)
{
	double orderSizeUSD = 0;
	int cc = SUCCESS;

	cc = ProcessWrapper(get_tradeable_balance,0);

	if (cc == SUCCESS)
	{
		orderSizeUSD = (marginBalance * percentMarginToUseDecimal) / MARGIN_MAX;

		if (orderSizeUSD > marginBalance_PL)
			cc =  INSUFFICENT_FUNDS;

		if (cc == SUCCESS)
		{
			*orderSize_out = orderSizeUSD / bfxLibrary.GetCurrentPrice();

			if (*orderSize_out < minimumOrderAmount)
				*orderSize_out = minimumOrderAmount;
		}

	}

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	return cc;
}

int BfxTradingBotMain::TakeProfit(double percentDecimal)
{
	int cc = SUCCESS;
	double positionSize = bfxLibrary.GetCurrentPositionAmount();
	double takeProfitAmount = positionSize * percentDecimal;
	double positionSizeAfterTakeProfit = positionSize - takeProfitAmount;
	double oppsiteAmount = takeProfitAmount * -1;//get oppsite value to take actual profit

	if (abs(positionSizeAfterTakeProfit) < minimumOrderAmount)
		return cc;//short circut just return success so we set the take profit flag but do nothing
	else
	{
		cc = CreateOrder(oppsiteAmount,false);//2nd param doesnt matter
	}
	return cc;
}

int BfxTradingBotMain::ClosePosition()
{
	int cc = SUCCESS;

	cc = ProcessWrapper(close_position, 0);

	return cc;
}

int BfxTradingBotMain::CreateOrder(double amount,bool long_order)
{
	int cc = SUCCESS;
	double amount_calc = 0;
	if(amount == 0)
		GetCreateOrderSize(&amount_calc);

	if (long_order != true && amount == 0)//we calculated our own amount flip based on param
		amount_calc *= -1;//not long do a short

	if (amount_calc != 0)
		amount = amount_calc;

	if (cc == SUCCESS)
		cc = ProcessWrapper(create_order, 1, amount);

	return cc;
}

int BfxTradingBotMain::ProcessWrapper(int type,int num_args,...)
{
	int cc = SUCCESS;
	int numTries = 0;
	int numRetriesAllowed = 10;

	va_list start;
	va_start(start, num_args);

	double amount = va_arg(start, double);

	do
	{
		switch (type)
		{
			case get_tradeable_balance:
			{
				cc = bfxLibrary.GetTradeableBalance(&marginBalance,&marginBalance_PL);
				break;
			}

			case create_order:
			{
				cc = bfxLibrary.CreateOrder(&amount);
				break;
			}

			case close_position:
			{
				cc = bfxLibrary.CreateClosePosition(&minimumOrderAmount);
				break;
			}
		
		}

		if (numTries < numRetriesAllowed)
		{
			if (cc == WebSocketGetDataTimeout || cc == WebSocketNotConnected)
			{
				cc = ReConnectWebSocketAuth();

				if (cc == SUCCESS)
				{
					cc = WebSocketGetDataTimeout;
				}

			}
		}

		numTries++;
	} while ((cc == WebSocketGetDataTimeout || cc == WebSocketNotConnected) && numTries < numRetriesAllowed);

	return cc;
}

int BfxTradingBotMain::GetMinimuimOrderAmount()
{
	int cc = SUCCESS;

	cc = bfxLibrary.GetMinimumOrderSize(&symbol, &minimumOrderAmount);

	return cc;
}


void BfxTradingBotMain::ResetTakeProfit()
{
	s1TakeProfit = false;
	s2TakeProfit = false;
	s3TakeProfit = false;
	r1TakeProfit = false;
	r2TakeProfit = false;
	r3TakeProfit = false;
}

int BfxTradingBotMain::ReadPersistentData()
{
	int cc = SUCCESS;
	fstream fileHandle;
	fileHandle.open(persisantDataPath, std::fstream::in | std::fstream::binary);
	if (fileHandle.is_open())
	{
		fileHandle.read(persistantDataStartPtr, persistentDataLength);
		fileHandle.close();
	}
	else
		cc = PERSISTENT_FILE_NOT_FOUND;
	return cc;
}

int BfxTradingBotMain::WritePersistentdata()
{
	int cc = SUCCESS;
	fstream fileHandle;
	fileHandle.open(persisantDataPath, std::fstream::out | std::fstream::binary);
	if (fileHandle.is_open())
	{
		fileHandle.write(persistantDataStartPtr, persistentDataLength);
		fileHandle.close();
	}
	else
		cc = PERSISTENT_FILE_NOT_FOUND;
	return cc;
}

int BfxTradingBotMain::CalculateWeeklyPivot()
{
	int cc = SUCCESS;

	cc = bfxLibrary.GetWeeklyPivot(&pivotPoint, &s1Short, &s2Short, &s3Short, &r1Long, &r2Long, &r3Long, &symbol);

	if (cc == SUCCESS)
		time(&lastPivotTime);

	return cc;
}

int BfxTradingBotMain::CalculateTimeFramePivot()
{
	int cc = SUCCESS;
	unsigned long long timeFrameBfx = 0;

	switch (candletimeFrame)
	{
		case ConfigTimeFrame::five_minute:
		{
			timeFrameBfx = ((futurePivotTime - DRIFT_OFFSET) - ((MINUTLY_SECS * 5)* 2));
			break;
		}
		case ConfigTimeFrame::fifteen_minute:
		{
			timeFrameBfx = (futurePivotTime - DRIFT_OFFSET) - ((MINUTLY_SECS * 15)* 2);
			break;
		}
		case ConfigTimeFrame::thirty_minute:
		{
			timeFrameBfx = (futurePivotTime - DRIFT_OFFSET) - ((MINUTLY_SECS * 30) * 2);
			break;
		}
		case ConfigTimeFrame::one_hour:
		{	
			timeFrameBfx = ((futurePivotTime - DRIFT_OFFSET) - (HOURLY_SECS *  2) );
			break;
		}
		case ConfigTimeFrame::three_hour:
		{
			timeFrameBfx = (futurePivotTime - DRIFT_OFFSET) - ((HOURLY_SECS * 3) * 2);
			break;
		}
		case ConfigTimeFrame::six_hour:
		{
			timeFrameBfx = (futurePivotTime - DRIFT_OFFSET) - ((HOURLY_SECS * 6) * 2);
			break;
		}
		case ConfigTimeFrame::twelve_hour:
		{
			timeFrameBfx = (futurePivotTime - DRIFT_OFFSET) - ((HOURLY_SECS * 12) * 2);
			break;
		}
		case ConfigTimeFrame::one_day:
		{
			timeFrameBfx = (futurePivotTime - DRIFT_OFFSET) - (DAILY_SECS * 2 );
			break;
		}
	}

	timeFrameBfx *= 1000;

	cc = bfxLibrary.GetTimeFramePivot(candletimeFrame, timeFrameBfx,&pivotPoint, &s1Short, &s2Short, &s3Short, &r1Long, &r2Long, &r3Long, &symbol);

	if (cc == SUCCESS)
		time(&lastPivotTime);

	return cc;
}

bool BfxTradingBotMain::IsTimeToCalcWeeklyPivot()
{
	time_t currTime = 0;
	time_t futureDayTime = 0;
	int numDaysAway = 0;
	time(&currTime);
	bool timeToCalc = true;

	if (currTime == lastPivotTime)
		return false;//we already did it in the same second


	if (currTime > futurePivotTime)//if its init to 0 or time to get new future time
	{
		struct tm * ptm_current;
		struct tm * ptm_future;
		ptm_current = gmtime(&currTime);

		if (ptm_current->tm_wday <= MONDAY)
			numDaysAway = MONDAY - ptm_current->tm_wday;
		else
			numDaysAway = 7 - (ptm_current->tm_wday - MONDAY);

		futureDayTime = currTime + (numDaysAway * DAILY_SECS);
		ptm_future = gmtime(&futureDayTime);

		ptm_future->tm_hour = 0;
		ptm_future->tm_min = 0;
		ptm_future->tm_sec = 8;//do 8 seconds after to account for any drift

		futurePivotTime = mktime(ptm_future) - timezone;

	}
	else
		timeToCalc = false;
	
	return timeToCalc;
}

bool BfxTradingBotMain::IsTimeToCalcTimeFramePivot()
{
	time_t currTime = 0;
	time_t futureDayTime = 0;
	struct tm * ptm_future;

	bool timeToCalc = true;
	time(&currTime);

	if (currTime == lastPivotTime)
		return false;//we already did it in the same second


	if (currTime > futurePivotTime)
	{
		if (candletimeFrame == ConfigTimeFrame::one_day)
		{
			futureDayTime = currTime + DAILY_SECS;
			ptm_future = gmtime(&futureDayTime);

			ptm_future->tm_hour = 0;
			ptm_future->tm_min = 0;
			ptm_future->tm_sec = DRIFT_OFFSET;//do 8 seconds after to account for any drift

			futurePivotTime = mktime(ptm_future) - timezone;
		}
		else
		{
			switch (candletimeFrame)
			{
				case ConfigTimeFrame::five_minute:
				{
					futureDayTime = currTime + (MINUTLY_SECS * 5);
					ptm_future = gmtime(&futureDayTime);
					ptm_future->tm_min = ptm_future->tm_min - (ptm_future->tm_min % 5);
					ptm_future->tm_sec = DRIFT_OFFSET;
					break;
				}
				case ConfigTimeFrame::fifteen_minute:
				{
					futureDayTime = currTime + (MINUTLY_SECS * 15);
					ptm_future = gmtime(&futureDayTime);
					ptm_future->tm_min = ptm_future->tm_min - (ptm_future->tm_min % 15);
					ptm_future->tm_sec = DRIFT_OFFSET;
					break;
				}
				case ConfigTimeFrame::thirty_minute:
				{
					futureDayTime = currTime + (MINUTLY_SECS * 30);
					ptm_future = gmtime(&futureDayTime);
					ptm_future->tm_min = ptm_future->tm_min - (ptm_future->tm_min % 30);
					ptm_future->tm_sec = DRIFT_OFFSET;
					break;
				}
				case ConfigTimeFrame::one_hour:
				{
					futureDayTime = currTime + HOURLY_SECS;
					ptm_future = gmtime(&futureDayTime);

					ptm_future->tm_min = 0;
					ptm_future->tm_sec = DRIFT_OFFSET;

					break;
				}
				case ConfigTimeFrame::three_hour:
				{
					futureDayTime = currTime + (HOURLY_SECS * 3);
					ptm_future = gmtime(&futureDayTime);
					ptm_future->tm_hour = ptm_future->tm_hour - (ptm_future->tm_hour % 3);

					ptm_future->tm_min = 0;
					ptm_future->tm_sec = DRIFT_OFFSET;

					break;
				}
				case ConfigTimeFrame::six_hour:
				{
					futureDayTime = currTime + (HOURLY_SECS * 6);
					ptm_future = gmtime(&futureDayTime);
					ptm_future->tm_hour = ptm_future->tm_hour - (ptm_future->tm_hour % 6);

					ptm_future->tm_min = 0;
					ptm_future->tm_sec = DRIFT_OFFSET;

					break;
				}
				case ConfigTimeFrame::twelve_hour:
				{
					futureDayTime = currTime + (HOURLY_SECS * 12);
					ptm_future = gmtime(&futureDayTime);
					ptm_future->tm_hour = ptm_future->tm_hour - (ptm_future->tm_hour % 12);

					ptm_future->tm_min = 0;
					ptm_future->tm_sec = DRIFT_OFFSET;

					break;
				}
			}

			
			futurePivotTime = mktime(ptm_future) - timezone;
		}
	}
	else
		timeToCalc = false;


	return timeToCalc;
}

int BfxTradingBotMain::Process()
{
	int cc = SUCCESS;

	if (bfxLibrary.GetWebSocketTradeChannelStatus() != ReadyState::Open || bfxLibrary.GetWebSocketAuthStatus() != ReadyState::Open)
	{
		cc = WebSocketConnectFailure; // since we arent connected just set it to failure so we do not log
		if (ErrorConnectingBfx == false)
			ErrorConnectingBfx = true;
		if (IsTimeToReconnect())
		{
			cc = ReConnectWebSocketAuth();
			if(cc == SUCCESS)
				cc = ReConnectWebSocketTradingChannel();
		}
			
		if (cc == SUCCESS)
			ErrorConnectingBfx = false;
	}


	switch (candletimeFrame)
	{
		case ConfigTimeFrame::five_minute:
		case ConfigTimeFrame::fifteen_minute:
		case ConfigTimeFrame::thirty_minute:
		case ConfigTimeFrame::one_hour:
		case ConfigTimeFrame::three_hour:
		case ConfigTimeFrame::six_hour:
		case ConfigTimeFrame::twelve_hour:
		case ConfigTimeFrame::one_day:
		{
			if (needPivotData || IsTimeToCalcTimeFramePivot())
			{
				if (needPivotData)//on start up
					IsTimeToCalcTimeFramePivot();//set future  time

				needPivotData = true;//new week lets get that data

				cc = CalculateTimeFramePivot();

				if (cc == SUCCESS)
				{
					if (firsTimePivotData == false)
					{
						ResetTakeProfit();
						WritePersistentdata();
					}
					firsTimePivotData = false;
					needPivotData = false;//start going through business logic after everything has been loaded
					LOG_TRACE(string_format(std::string("Calculated pivotdata  : { %lf,%lf,%lf,%lf,%lf,%lf,%lf }"), pivotPoint, r1Long, r2Long, r3Long,
						s1Short, s2Short, s3Short).c_str());
				}
			}
				
				
			break;
		}

		case ConfigTimeFrame::one_week://fall thru to default
		default:
		{
			if (needPivotData || IsTimeToCalcWeeklyPivot())
			{
				if (needPivotData)//on start up
					IsTimeToCalcWeeklyPivot();//set future  time

				needPivotData = true;//new week lets get that data

				cc = CalculateWeeklyPivot();

				if (cc == SUCCESS)
				{
					if (firsTimePivotData == false)
					{
						ResetTakeProfit();
						WritePersistentdata();
					}
					firsTimePivotData = false;
					needPivotData = false;//start going through business logic after everything has been loaded
					LOG_TRACE(string_format(std::string("Calculated pivotdata  : { %lf,%lf,%lf,%lf,%lf,%lf,%lf }"), pivotPoint, r1Long, r2Long, r3Long,
						s1Short, s2Short, s3Short).c_str());
				}
			}
				
		}
	}
	

	if (IsTimeToGetMinimumOrderAmount() || minimumOrderAmount == -1)
	{
		cc = GetMinimuimOrderAmount();

	}

	return cc;
}