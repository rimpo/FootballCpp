#include "stdafx.h"
#include "Game.h"



CGame::CGame()
{
	//creating strategy
	strategyPtr_ = make_shared<CCounterAttackerAI>();

	strategyPtr_->CreateAllPlayers();

	//copying over all the players created by strategy. (helps in creating custom player)
	allPlayers_ = strategyPtr_->GetAllPlayers();
	

	//creating team (our and their)
	ourTeamPtr_ = make_shared<CTeam>();
	theirTeamPtr_ = make_shared<CTeam>();

	teams_.push_back(ourTeamPtr_);
	teams_.push_back(theirTeamPtr_);
		
	noOfGoalAttempts = 0;
}


CGame::~CGame()
{
	
}


int CGame::Process(const string& sJsonMsg)
{
	Document document;

	if (document.Parse(sJsonMsg.c_str()).HasParseError() == true)
	{
		return -1;
	}
	
	const Value& messageTypeValue = document["messageType"];

	Value::MemberIterator eventTypeItr = document.FindMember("eventType");
	
	if (eventTypeItr != document.MemberEnd())
	{
		//Note: More frequently received event should be above in if condition.
		//      This is to avoiding if else check.

		if (eventTypeItr->value == "START_OF_TURN")
		{
			currentTimeSeconds_ = document["game"]["currentTimeSeconds"].GetDouble();

			ball_.ProcessStartOfTurn(document["ball"]);

			ourTeamPtr_->ProcessStartOfTurn(document);
			theirTeamPtr_->ProcessStartOfTurn(document);

			strategyPtr_->OnStartOfTurnEvent();
		}
		else if (eventTypeItr->value == "GOAL")
		{
			strategyPtr_->OnGoalEvent();
		}
		else if (eventTypeItr->value == "HALF_TIME")
		{
			strategyPtr_->OnHalfTimeEvent();
		}
		else if (eventTypeItr->value == "KICKOFF")
		{
			ourTeamPtr_->ProcessTeamKickOffInfo(document);
			theirTeamPtr_->ProcessTeamKickOffInfo(document);

			strategyPtr_->OnKickOffEvent();
		}
		else if (eventTypeItr->value == "TEAM_INFO")
		{
			ourTeamPtr_->ProcessTeamInfo(document);

			//setting theirTeam identifier since we dont receive in team info.
			if (ourTeamPtr_->GetTeamIdentifier() == "team1")
			{
				theirTeamPtr_->SetTeamNumber(2);
				theirTeamPtr_->SetTeamIdentifier("team2");
			}
			else
			{
				theirTeamPtr_->SetTeamNumber(1);
				theirTeamPtr_->SetTeamIdentifier("team1");
			}

			strategyPtr_->OnTeamInfoEvent();
		}
		else if (eventTypeItr->value == "GAME_START")
		{
			pitch_.ProcessPitch(document["pitch"]);

			gameLengthSeconds_ = document["gameLengthSeconds"].GetDouble();

			strategyPtr_->OnGameStartEvent();
		}
		

	}
	else
	{
		//Note: More frequently received request should be above in if condition.
		//      This is to avoiding if else check.

		const Value& requestTypeValue = document["requestType"];

		if (requestTypeValue == "PLAY")
		{
			strategyPtr_->OnPlayRequest();

			PrintPlayResponse();
		}
		else if (requestTypeValue == "KICKOFF")
		{
			strategyPtr_->OnKickOffRequest();

			PrintKickOffResponse();
		}
		else if (requestTypeValue == "CONFIGURE_ABILITIES")
		{
			ourTeamPtr_->ProcessCapabilitiesRequest(document);

			strategyPtr_->OnCapabilityRequest();

			PrintCapabilityResponse();
		}
	}

	return 0;
}
void CGame::PrintCapabilityResponse()
{
	/*template <class TYPE> std::string Str(const TYPE & t) {
		std::ostringstream os;
		os << t;
		return os.str();
	}*/

	//{"requestType":"CONFIGURE_ABILITIES", "players" : [{"playerNumber":6, "kickingAbility" : 66.6667, "runningAbility" : 66.6667, "ballControlAbility" : 66.6667, "tacklingAbility" : 66.6667}, { "playerNumber":7, "kickingAbility" : 66.6667, "runningAbility" : 66.6667, "ballControlAbility" : 66.6667, "tacklingAbility" : 66.6667 }, { "playerNumber":8, "kickingAbility" : 66.6667, "runningAbility" : 66.6667, "ballControlAbility" : 66.6667, "tacklingAbility" : 66.6667 }, { "playerNumber":9, "kickingAbility" : 66.6667, "runningAbility" : 66.6667, "ballControlAbility" : 66.6667, "tacklingAbility" : 66.6667 }, { "playerNumber":10, "kickingAbility" : 66.6667, "runningAbility" : 66.6667, "ballControlAbility" : 66.6667, "tacklingAbility" : 66.6667 }, { "playerNumber":11, "kickingAbility" : 66.6667, "runningAbility" : 66.6667, "ballControlAbility" : 66.6667, "tacklingAbility" : 66.6667 }]}
	cout << "{\"requestType\":\"CONFIGURE_ABILITIES\",\"players\" : [";

	const CPlayer::PtrVec& ourTeamPlayers = ourTeamPtr_->GetPlayers();

	int i = 0;
	for (auto pPlayer : ourTeamPlayers)
	{
		if (0 != i++)
			cout << ",";

		cout << "{";
		cout << "\"playerNumber\":" << pPlayer->GetNumber() << ",";

		const CCapability& cap = pPlayer->GetCapability();

		cout << "\"kickingAbility\":" << cap.kickingAbility_ << ",";
		cout << "\"runningAbility\":" << cap.runningAbility_ << ",";
		cout << "\"ballControlAbility\":" << cap.ballControlAbility_ << ",";
		cout << "\"tacklingAbility\":" << cap.tacklingAbility_ << "";
		
		cout << "}";
	}

	cout << "]}" << endl;

}
void CGame::PrintKickOffResponse()
{
	//Need for flipping co-ordinates
	CPitch& pitch = GetGame().GetPitch();
	const CTeam::Ptr& ourTeamPtr = GetGame().GetOurTeamPtr();
		
	cout << "{\"requestType\":\"KICKOFF\", \"players\" : [";

	const CPlayer::PtrVec& ourTeamPlayers = ourTeamPtr_->GetPlayers();

	Position pos;
	float direction;
	int i = 0;
	for (auto pPlayer : ourTeamPlayers)
	{
		if (0 != i++)
			cout << ",";

		pos = pPlayer->GetKickOffPosition();
		direction = pPlayer->GetKickOffDirection();

		cout << "{\"playerNumber\":" << pPlayer->GetNumber();
		
		//flip co-ordinates
		if (ourTeamPtr->GetPlayingDirection() == CTeam::eLeft)
		{
			pos.x_ = pitch.GetWidth() - pos.x_;
			direction = 360.0 - direction;
		}

		cout << ",\"position\":{\"x\":" << pos.x_
			<< ",\"y\":" << pos.y_;

		cout << "},\"direction\":" << direction;
		
		cout << "}";
	}
	cout << "]}" << endl;
}
void CGame::PrintPlayResponse()
{
	//Need for flipping co-ordinates
	CPitch& pitch = GetGame().GetPitch();
	const CTeam::Ptr& ourTeamPtr = GetGame().GetOurTeamPtr();

	cout << "{\"requestType\":\"PLAY\",\"actions\":[";
	const CPlayer::PtrVec& ourTeamPlayers = ourTeamPtr_->GetPlayers();

	Position destination;
	float	 direction = 0.0;
	int i = 0;
	for (auto pPlayer : ourTeamPlayers)
	{
		CAction& action = pPlayer->GetAction();

		if (action.type_ == CAction::eNoAction)
			continue;

		if (0 != i++)
			cout << ",";

		destination = action.destination_;
		direction = action.direction_;

		//flip co-ordinates
		if (ourTeamPtr->GetPlayingDirection() == CTeam::eLeft)
		{
			destination.x_ = pitch.GetWidth() - destination.x_;
			direction = 360.0 - direction;
		}

		cout << "{\"playerNumber\":" << pPlayer->GetNumber();
		cout << ",\"action\":" << action.GetActionString();

		switch (action.type_)
		{
		case CAction::eKick:
		case CAction::eMove:
					
			cout << ",\"destination\":{ \"x\":" << destination.x_
				<< ",\"y\":" << destination.y_ << "}";

			cout << ",\"speed\":" << action.speed_;
			break;
		case CAction::eTurn:
			cout << ",\"direction\":" << direction; 
			break;
		}

		cout << "}";
	}
	cout << "]}" << endl;
}

void CGame::CalculatePlayerReachesFirstToBall()
{
	//ball is inside goal area.
	if (pitch_.IsInsideTheirGoalArea(ball_.GetPosition()))
	{
		closestPlayer_ = ourTeamPtr_->GetGoalKeeper();
		return;
	}
	else if (pitch_.IsInsideOurGoalArea(ball_.GetPosition()))
	{
		closestPlayer_ = theirTeamPtr_->GetGoalKeeper();
		return;
	}
	
	auto ourNonGoalKeeper = ourTeamPtr_->GetNonGoalKeepers();

	double minTimeTaken = 9999999.0;	//dummy max number
	double t = 0.0;

	//all non goal keeper (from both teams)
	for (auto teamPtr : teams_)
	{
		auto nonGoalKeepers = teamPtr->GetNonGoalKeepers();
		for (auto pPlayer : nonGoalKeepers)
		{
			t = pPlayer->CalculateTimeToReachPosition(ball_.GetPosition());

			if (minTimeTaken > t)
			{
				minTimeTaken = t;
				closestPlayer_ = pPlayer;
			}
		}
	}

	return;
}

//sort their team with respect to x cordinate (ascending order)
void CGame::SortTheirTeamX()
{
	//copy their non-goalkeeper players into a vector if empty
	if (theirTeamSortedX_.empty())
	{
		theirTeamSortedX_ = theirTeamPtr_->GetNonGoalKeepers();
	}

	//sort the vector.
	sort(begin(theirTeamSortedX_), end(theirTeamSortedX_),
		[](const CPlayer::Ptr & a, const CPlayer::Ptr & b) -> bool
	{
		return a->GetPosition().x_ < b->GetPosition().x_;
	});
}