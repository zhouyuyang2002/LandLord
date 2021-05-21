#include <iostream>
#include <ctime> 
#include <set>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <cassert>
#include <cstring> // ×¢ÒâmemsetÊÇcstringÀïµÄ
#include <algorithm>
#include "jsoncpp/json.h" // ÔÚÆ½Ì¨ÉÏ£¬C++±àÒëÊ±Ä¬ÈÏ°üº¬´Ë¿â


using namespace std;


using std::set;
using std::sort;
using std::string;
using std::unique;
using std::vector;

constexpr int PLAYER_COUNT = 3;

enum class Stage
{
	BIDDING, // ½Ð·Ö½×¶Î
	PLAYING	 // ´òÅÆ½×¶Î
};

enum class CardComboType
{
	PASS,		// ¹ý
	SINGLE,		// µ¥ÕÅ
	PAIR,		// ¶Ô×Ó
	STRAIGHT,	// Ë³×Ó
	STRAIGHT2,	// Ë«Ë³
	TRIPLET,	// ÈýÌõ
	TRIPLET1,	// Èý´øÒ»
	TRIPLET2,	// Èý´ø¶þ
	BOMB,		// Õ¨µ¯
	QUADRUPLE2, // ËÄ´ø¶þ£¨Ö»£©
	QUADRUPLE4, // ËÄ´ø¶þ£¨¶Ô£©
	PLANE,		// ·É»ú
	PLANE1,		// ·É»ú´øÐ¡Òí
	PLANE2,		// ·É»ú´ø´óÒí
	SSHUTTLE,	// º½Ìì·É»ú
	SSHUTTLE2,	// º½Ìì·É»ú´øÐ¡Òí
	SSHUTTLE4,	// º½Ìì·É»ú´ø´óÒí
	ROCKET,		// »ð¼ý
	INVALID		// ·Ç·¨ÅÆÐÍ
};

#ifndef _BOTZONE_ONLINE
string cardComboStrings[] = {
	"PASS",
	"SINGLE",
	"PAIR",
	"STRAIGHT",
	"STRAIGHT2",
	"TRIPLET",
	"TRIPLET1",
	"TRIPLET2",
	"BOMB",
	"QUADRUPLE2",
	"QUADRUPLE4",
	"PLANE",
	"PLANE1",
	"PLANE2",
	"SSHUTTLE",
	"SSHUTTLE2",
	"SSHUTTLE4",
	"ROCKET",
	"INVALID"};
#endif

// ÓÃ0~53Õâ54¸öÕûÊý±íÊ¾Î¨Ò»µÄÒ»ÕÅÅÆ
using Card = short;
constexpr Card card_joker = 52;
constexpr Card card_JOKER = 53;

// ³ýÁËÓÃ0~53Õâ54¸öÕûÊý±íÊ¾Î¨Ò»µÄÅÆ£¬
// ÕâÀï»¹ÓÃÁíÒ»ÖÖÐòºÅ±íÊ¾ÅÆµÄ´óÐ¡£¨²»¹Ü»¨É«£©£¬ÒÔ±ã±È½Ï£¬³Æ×÷µÈ¼¶£¨Level£©
// ¶ÔÓ¦¹ØÏµÈçÏÂ£º
// 3 4 5 6 7 8 9 10	J Q K	A	2	Ð¡Íõ	´óÍõ
// 0 1 2 3 4 5 6 7	8 9 10	11	12	13	14
using Level = short;
constexpr Level MAX_LEVEL = 15;
constexpr Level MAX_STRAIGHT_LEVEL = 11;
constexpr Level level_joker = 13;
constexpr Level level_JOKER = 14;

/**
* ½«Card±ä³ÉLevel
*/
constexpr Level card2level(Card card){
	return card / 4 + card / 53;
}

// ÅÆµÄ×éºÏ£¬ÓÃÓÚ¼ÆËãÅÆÐÍ
struct CardCombo
{
	// ±íÊ¾Í¬µÈ¼¶µÄÅÆÓÐ¶àÉÙÕÅ
	// »á°´¸öÊý´Ó´óµ½Ð¡¡¢µÈ¼¶´Ó´óµ½Ð¡ÅÅÐò
	struct CardPack
	{
		Level level;
		short count;

		bool operator<(const CardPack &b) const
		{
			if (count == b.count)
				return level > b.level;
			return count > b.count;
		}
	};
	vector<Card> cards;		 // Ô­Ê¼µÄÅÆ£¬Î´ÅÅÐò
	vector<CardPack> packs;	 // °´ÊýÄ¿ºÍ´óÐ¡ÅÅÐòµÄÅÆÖÖ
	CardComboType comboType; // Ëã³öµÄÅÆÐÍ
	Level comboLevel = 0;	 // Ëã³öµÄ´óÐ¡Ðò

	/**
						  * ¼ì²é¸öÊý×î¶àµÄCardPackµÝ¼õÁË¼¸¸ö
						  */
	int findMaxSeq() const
	{
		for (unsigned c = 1; c < packs.size(); c++)
			if (packs[c].count != packs[0].count ||
				packs[c].level != packs[c - 1].level - 1)
				return c;
		return packs.size();
	}

	// ´´½¨Ò»¸ö¿ÕÅÆ×é
	CardCombo() : comboType(CardComboType::PASS) {}

	/**
	* Í¨¹ýCard£¨¼´short£©ÀàÐÍµÄµü´úÆ÷´´½¨Ò»¸öÅÆÐÍ
	* ²¢¼ÆËã³öÅÆÐÍºÍ´óÐ¡ÐòµÈ
	* ¼ÙÉèÊäÈëÃ»ÓÐÖØ¸´Êý×Ö£¨¼´ÖØ¸´µÄCard£©
	*/
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		// ÌØÅÐ£º¿Õ
		if (begin == end)
		{
			comboType = CardComboType::PASS;
			return;
		}

		// Ã¿ÖÖÅÆÓÐ¶àÉÙ¸ö
		short counts[MAX_LEVEL + 1] = {};

		// Í¬ÖÖÅÆµÄÕÅÊý£¨ÓÐ¶àÉÙ¸öµ¥ÕÅ¡¢¶Ô×Ó¡¢ÈýÌõ¡¢ËÄÌõ£©
		short countOfCount[5] = {};

		cards = vector<Card>(begin, end);
		for (Card c : cards)
			counts[card2level(c)]++;
		for (Level l = 0; l <= MAX_LEVEL; l++)
			if (counts[l])
			{
				packs.push_back(CardPack{l, counts[l]});
				countOfCount[counts[l]]++;
			}
		sort(packs.begin(), packs.end());

		// ÓÃ×î¶àµÄÄÇÖÖÅÆ×ÜÊÇ¿ÉÒÔ±È½Ï´óÐ¡µÄ
		comboLevel = packs[0].level;

		// ¼ÆËãÅÆÐÍ
		// °´ÕÕ Í¬ÖÖÅÆµÄÕÅÊý ÓÐ¼¸ÖÖ ½øÐÐ·ÖÀà
		vector<int> kindOfCountOfCount;
		for (int i = 0; i <= 4; i++)
			if (countOfCount[i])
				kindOfCountOfCount.push_back(i);
		sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

		int curr, lesser;

		switch (kindOfCountOfCount.size())
		{
		case 1: // Ö»ÓÐÒ»ÀàÅÆ
			curr = countOfCount[kindOfCountOfCount[0]];
			switch (kindOfCountOfCount[0])
			{
			case 1:
				// Ö»ÓÐÈô¸Éµ¥ÕÅ
				if (curr == 1)
				{
					comboType = CardComboType::SINGLE;
					return;
				}
				if (curr == 2 && packs[1].level == level_joker)
				{
					comboType = CardComboType::ROCKET;
					return;
				}
				if (curr >= 5 && findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::STRAIGHT;
					return;
				}
				break;
			case 2:
				// Ö»ÓÐÈô¸É¶Ô×Ó
				if (curr == 1)
				{
					comboType = CardComboType::PAIR;
					return;
				}
				if (curr >= 3 && findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::STRAIGHT2;
					return;
				}
				break;
			case 3:
				// Ö»ÓÐÈô¸ÉÈýÌõ
				if (curr == 1)
				{
					comboType = CardComboType::TRIPLET;
					return;
				}
				if (findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::PLANE;
					return;
				}
				break;
			case 4:
				// Ö»ÓÐÈô¸ÉËÄÌõ
				if (curr == 1)
				{
					comboType = CardComboType::BOMB;
					return;
				}
				if (findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::SSHUTTLE;
					return;
				}
			}
			break;
		case 2: // ÓÐÁ½ÀàÅÆ
			curr = countOfCount[kindOfCountOfCount[1]];
			lesser = countOfCount[kindOfCountOfCount[0]];
			if (kindOfCountOfCount[1] == 3)
			{
				// ÈýÌõ´ø£¿
				if (kindOfCountOfCount[0] == 1)
				{
					// Èý´øÒ»
					if (curr == 1 && lesser == 1)
					{
						comboType = CardComboType::TRIPLET1;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::PLANE1;
						return;
					}
				}
				if (kindOfCountOfCount[0] == 2)
				{
					// Èý´ø¶þ
					if (curr == 1 && lesser == 1)
					{
						comboType = CardComboType::TRIPLET2;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::PLANE2;
						return;
					}
				}
			}
			if (kindOfCountOfCount[1] == 4)
			{
				// ËÄÌõ´ø£¿
				if (kindOfCountOfCount[0] == 1)
				{
					// ËÄÌõ´øÁ½Ö» * n
					if (curr == 1 && lesser == 2)
					{
						comboType = CardComboType::QUADRUPLE2;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr * 2 &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::SSHUTTLE2;
						return;
					}
				}
				if (kindOfCountOfCount[0] == 2)
				{
					// ËÄÌõ´øÁ½¶Ô * n
					if (curr == 1 && lesser == 2)
					{
						comboType = CardComboType::QUADRUPLE4;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr * 2 &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::SSHUTTLE4;
						return;
					}
				}
			}
		}

		comboType = CardComboType::INVALID;
	}

	/**
	* ÅÐ¶ÏÖ¸¶¨ÅÆ×éÄÜ·ñ´ó¹ýµ±Ç°ÅÆ×é£¨Õâ¸öº¯Êý²»¿¼ÂÇ¹ýÅÆµÄÇé¿ö£¡£©
	*/
	bool canBeBeatenBy(const CardCombo &b) const
	{
		if (comboType == CardComboType::INVALID || b.comboType == CardComboType::INVALID)
			return false;
		if (b.comboType == CardComboType::ROCKET)
			return true;
		if (b.comboType == CardComboType::BOMB)
			switch (comboType)
			{
			case CardComboType::ROCKET:
				return false;
			case CardComboType::BOMB:
				return b.comboLevel > comboLevel;
			default:
				return true;
			}
		return b.comboType == comboType && b.cards.size() == cards.size() && b.comboLevel > comboLevel;
	}


	void debugPrint()
	{
#ifndef _BOTZONE_ONLINE
		std::cout << "¡¾" << cardComboStrings[(int)comboType] << "¹²" << cards.size() << "ÕÅ£¬´óÐ¡Ðò" << comboLevel << "¡¿";
#endif
	}
};

/* ×´Ì¬ */
// ÎÒµÄÅÆÓÐÄÄÐ©
set<Card> myCards;
// µØÖ÷Ã÷Ê¾µÄÅÆÓÐÄÄÐ©
set<Card> landlordPublicCards;
// ´ó¼Ò´Ó×î¿ªÊ¼µ½ÏÖÔÚ¶¼³ö¹ýÊ²Ã´
vector<vector<Card>> whatTheyPlayed[PLAYER_COUNT];
// µ±Ç°Òª³öµÄÅÆÐèÒª´ó¹ýË­
CardCombo lastValidCombo;
// µ±Ç°Òª´ó¹ýµÄÅÆÊÇË­³öµÄ 
int lastPosition=-1;
// ´ó¼Ò»¹Ê£¶àÉÙÅÆ
short cardRemaining[PLAYER_COUNT] = {17, 17, 17};
// ÎÒÊÇ¼¸ºÅÍæ¼Ò£¨0-µØÖ÷£¬1-Å©Ãñ¼×£¬2-Å©ÃñÒÒ£©
int myPosition;
// µØÖ÷Î»ÖÃ
int landlordPosition = -1;
// µØÖ÷½Ð·Ö
int landlordBid = -1;
// ½×¶Î
Stage stage = Stage::BIDDING;

// ×Ô¼ºµÄµÚÒ»»ØºÏÊÕµ½µÄ½Ð·Ö¾ö²ß
vector<int> bidInput;

//Õâ²¿·ÖÓÃÓÚ½Ð·ÖÊ±¿ÌµÄ¾ö²ß¡£
namespace Envaluator_inital{
	
	bool initized=false;
	struct MyCardCombo{
		CardComboType ComboType;
		Level ComboLevel;
		int Combolen;
	}; 
	/*3 4 5 6 7 8 9 10 J Q K  A  2  J1 J2*/
	/*0 1 2 3 4 5 6 7  8 9 10 11 12 13 14*/
	int level_num[20],highest_score;
	vector<Level> Combo_level[5];
	set<Level> CopyCombo[5];
	
	int Punish[50005];
	const int min_straight_len[]={
		0,5,3,2
	};//Ë³×Ó×îÐ¡µÄ³¤¶È
	
	int F(int level){
		return (int)(pow(1.37,level));
	}
	int G(int len){
		return max(len*2,(int)(pow(1.58,len)));
	}
	int H(int level){
		return max((15-level)*2,(int)(pow(1.37,15-level)));
	}
	int Scoreing(vector<MyCardCombo> Combos){
		int ans=0;
		vector<int> temp;
		for (auto i:Combos){
			if (i.ComboType==CardComboType::SINGLE)
				temp.push_back(1+2*F(i.ComboLevel));
			if (i.ComboType==CardComboType::PAIR)
				temp.push_back(3+3*F(i.ComboLevel));
			if (i.ComboType==CardComboType::STRAIGHT)
				temp.push_back(1+6*F(i.ComboLevel)+3*G(i.Combolen));
			if (i.ComboType==CardComboType::STRAIGHT2)
				temp.push_back(1+6*F(i.ComboLevel)+5*G(i.Combolen));
			if (i.ComboType==CardComboType::TRIPLET)
				temp.push_back(8+8*F(i.ComboLevel));
			if (i.ComboType==CardComboType::TRIPLET1)
				temp.push_back(12+8*F(i.ComboLevel));
			if (i.ComboType==CardComboType::TRIPLET2)
				temp.push_back(18+8*F(i.ComboLevel));
			if (i.ComboType==CardComboType::BOMB)
				temp.push_back(50+20*F(i.ComboLevel));
			if (i.ComboType==CardComboType::QUADRUPLE2)
				temp.push_back(100+20*F(i.ComboLevel));
			if (i.ComboType==CardComboType::QUADRUPLE4)
				temp.push_back(150+20*F(i.ComboLevel));
			if (i.ComboType==CardComboType::PLANE)
				temp.push_back(1+8*G(i.Combolen)+6*F(i.ComboLevel));
			if (i.ComboType==CardComboType::PLANE1)
				temp.push_back(1+10*G(i.Combolen)+6*F(i.ComboLevel));
			if (i.ComboType==CardComboType::PLANE2)
				temp.push_back(1+12*G(i.Combolen)+6*F(i.ComboLevel));
			if (i.ComboType==CardComboType::ROCKET)
				temp.push_back(800);
		}
		int result=-pow(2,2.5*temp.size()/2); /*³öÅÆ´ÎÊý¹ý¶àÓÐ³Í·£*/
		for (auto i:temp) result+=i-Punish[i];
		if (Combos.size()>=1){
			int lv=0;
			for (auto i:Combos)
				lv=max(lv,(int)i.ComboLevel);
			//cout<<lv<<endl;
			result-=H(lv);
		}
		/*static int fir=0;
		if (fir==0){
			fir=1;
			cout<<"1145141919810"<<' '<<result<<endl;
			for (auto i:Combos) cout<<cardComboStrings[(int)i.ComboType]<<' '<<i.ComboLevel<<' '<<endl; 
			
		}*/
		return result;
	}
	
	bool Legal_2(int ned){
		if (CopyCombo[2].size()<ned) return 0;
		for (;ned;CopyCombo[2].erase(CopyCombo[2].begin()),--ned);
		return 1;
	}
	bool Legal_1(int ned){
		if (CopyCombo[0].size()+CopyCombo[1].size()<ned) return 0;
		vector<int> erased;
		for (;ned&&CopyCombo[0].size();--ned){
			int val=*CopyCombo[0].begin();
			CopyCombo[0].erase(val);
			erased.push_back(val);
		}
		for (;ned;CopyCombo[1].erase(CopyCombo[1].begin()),--ned);
		for (auto i:erased) CopyCombo[1].insert(i);
		return 1;
	}
	
	//Ì°ÐÄ´¦ÀíÈý´øÒ»£¬Èý´ø¶þµÄÇé¿ö
	void Update(vector<MyCardCombo> Combos,int SANDAI1,int SIDAI1,int BOMB){
		//cout<<"Update"<<' '<<SANDAI1<<' '<<SIDAI1<<endl;
		//static 
		for (int i=0;i<=4;i++){
			CopyCombo[i].clear();
			for (auto j:Combo_level[i])
				CopyCombo[i].insert(j);
		}
		for (auto &i:Combos)
			if (i.ComboType==CardComboType::TRIPLET){
				if (SANDAI1){
					--SANDAI1;
					if (Legal_1(1)) i.ComboType=CardComboType::TRIPLET1;
				}
				else
					if (Legal_2(1)) i.ComboType=CardComboType::TRIPLET2;
			}
			else if (i.ComboType==CardComboType::BOMB){
				if (BOMB){
					--BOMB;
					continue;
				}
				if (SIDAI1){
					--SIDAI1;
					if (Legal_1(2)) i.ComboType=CardComboType::QUADRUPLE2;
				}
				else
					if (Legal_2(2)) i.ComboType=CardComboType::QUADRUPLE4;
			}
		for (auto i:CopyCombo[0])
			Combos.push_back((MyCardCombo){CardComboType::PAIR,i,-1});
		for (auto i:CopyCombo[1])
			Combos.push_back((MyCardCombo){CardComboType::SINGLE,i,-1});
		for (auto i:CopyCombo[2])
			Combos.push_back((MyCardCombo){CardComboType::PAIR,i,-1});
		highest_score=max(highest_score,Scoreing(Combos));
		
		//cout<<"UpdateE"<<' '<<SANDAI1<<' '<<SIDAI1<<endl;
	}
	//ÕâÀïÎÒÃÇÈÏÎªÎÒÃÇ²»»áÌØ±ðµÄÅ·»Ê£¬Ñ¡µ½º½Ìì·É»ú ÕâÀïÑ¡Ôñ²»ÅÐ¡£
	//ÌáÈ¡³öÀ´Ã»ÓÐstraightµÄÅÆÐÍºÅ
	//ÕâÀïÎÒÃÇÈÏÎª²»»á²ðµôÐÎÈç 222 µÄÅÆÐÍ
	void no_straight(vector<MyCardCombo> Combos){
		//cout<<"no_Str"<<endl;
		for (int i=0;i<=4;i++)
			Combo_level[i].resize(0);
		int S3=0,S4=0;
		for (Level i=0;i<=14;i++)
			switch (level_num[i]){
				case 0:break;
				case 1:
				case 2:
					if (i<=12)
						Combo_level[level_num[i]].push_back(i);
					else
						Combos.push_back((MyCardCombo){CardComboType::SINGLE,i,-1});
					break;
				case 3:
					Combo_level[level_num[i]].push_back(i);
					Combos.push_back((MyCardCombo){CardComboType::TRIPLET,i,-1});
					break;
				case 4:
					Combo_level[level_num[i]].push_back(i);
					Combos.push_back((MyCardCombo){CardComboType::BOMB,i,-1});
					break;
			}
		//ÍõÕ¨
		if (level_num[13]&&level_num[14]){
			Combos.pop_back();
			Combos.pop_back();
			Combos.push_back((MyCardCombo){CardComboType::ROCKET,14,-1});
		}
		swap(Combo_level[0],Combo_level[2]);
		for (;;){
			for (int i=0;i<=Combo_level[3].size();i++)
				for (int j=0;j<=Combo_level[4].size();j++)
					for (int k=0;k+j<=Combo_level[4].size();k++)
						Update(Combos,i,j,k);
			if (!Combo_level[0].size()) break;
			Combo_level[2].push_back(Combo_level[0].back());
			Combo_level[0].pop_back();
		}
		//cout<<"no_StrE"<<endl;
	}
	void dfs_straight(int,int,vector<MyCardCombo>);
	//·É»ú³á°ò
	void dfs_plane(int straight_num,int straight_ind,int num,int rem,int ind,vector<MyCardCombo> Combos){
		//cout<<"d_plane"<<endl;
		if (!rem){
			//cout<<"d_planeE"<<endl;
			return dfs_straight(straight_num,straight_ind,Combos);
		}
		int l=Combos.back().ComboLevel;
		int r=l+Combos.back().Combolen-1;
		for (;;){
			if (ind==15) break;
			if (level_num[ind]>=num&&((ind<l)||(ind>r))){
				level_num[ind]-=num;
				dfs_plane(straight_num,straight_ind,num,rem-1,ind+1,Combos);
				level_num[ind]+=num;
			}
			ind++;
		}
		//cout<<"d_planeE"<<endl;
	}
	//ËÑ³öÀ´ËùÓÐµÄË³×Ó
	void dfs_straight(int straight_num,int straight_ind,vector<MyCardCombo> Combos){
		//cout<<straight_num<<' '<<straight_ind<<endl;
		if (straight_num>=4)
			return no_straight(Combos);
		for (;;){
			if (straight_ind+min_straight_len[straight_num]-1>MAX_STRAIGHT_LEVEL) break;
			bool avaliable=true;
			int loc=straight_ind;
			for (int i=0;i<min_straight_len[straight_num];i++){
				if (level_num[loc]<straight_num){
					avaliable=false;
					break;
				}
				++loc;
			}
			if (avaliable){
				int loc=straight_ind;
				for (int i=0;i<min_straight_len[straight_num];i++){
					level_num[loc]-=straight_num;
					++loc;
				}
				for (;;){
					switch (straight_num){
						//Ë³×Ó
						case 1:
							Combos.push_back((MyCardCombo){CardComboType::STRAIGHT,straight_ind,loc-straight_ind});
							dfs_straight(straight_num,straight_ind,Combos);
							Combos.pop_back();
							break;
						//Á¬¶Ô
						case 2:
							Combos.push_back((MyCardCombo){CardComboType::STRAIGHT2,straight_ind,loc-straight_ind});
							dfs_straight(straight_num,straight_ind,Combos);
							Combos.pop_back();
							break;
						//·É»ú
						case 3:
							Combos.push_back((MyCardCombo){CardComboType::PLANE,straight_ind,loc-straight_ind});
							dfs_straight(straight_num,straight_ind,Combos);
							Combos.pop_back();
						  //·É»úÐ¡³á°ò
						  	Combos.push_back((MyCardCombo){CardComboType::PLANE1,straight_ind,loc-straight_ind});
						  	dfs_plane(straight_num,straight_ind,1,loc-straight_ind,0,Combos);
						  	Combos.pop_back();
						  //·É»ú´ó³á°ò
						  	Combos.push_back((MyCardCombo){CardComboType::PLANE2,straight_ind,loc-straight_ind});
						  	dfs_plane(straight_num,straight_ind,2,loc-straight_ind,0,Combos);
						  	Combos.pop_back();
						  	break;
					}
					if (loc>MAX_STRAIGHT_LEVEL||level_num[loc]<straight_num)
						break;
					level_num[loc]-=straight_num;
					++loc;
				}
				for (--loc;loc>=straight_ind;--loc)
					level_num[loc]+=straight_num;
			}
			++straight_ind;
		}
		dfs_straight(straight_num+1,0,Combos);
	}
	int envaluate(set<Card> S){
		if (!S.size())
			return 10000;
		if (!initized){
			initized=true;
			for (int i=1;i<=50000;i++) Punish[i]=(int)(80/sqrt(i));
		}
		//for (auto i:S) cout<<card2level(i)<<endl;
		vector<MyCardCombo> Combos;
		memset(level_num,0,sizeof(level_num));
		for (auto i:S) level_num[card2level(i)]+=1;
		highest_score=-(1<<30);
		dfs_straight(1,0,Combos);
		return highest_score;
	}
	
	int envaluate_ver_temp(multiset<Card> S){
		if (!S.size())
			return 10000;
		if (!initized){
			initized=true;
			for (int i=1;i<=50000;i++) Punish[i]=(int)(80/sqrt(i));
		}
		vector<MyCardCombo> Combos;
		memset(level_num,0,sizeof(level_num));
		for (auto i:S) level_num[i]+=1;
		highest_score=-(1<<30);
		dfs_straight(1,0,Combos);
		return highest_score;
	}
}
namespace BotzoneIO
{
	using namespace std;
	void read()
	{
		// ¶ÁÈëÊäÈë£¨Æ½Ì¨ÉÏµÄÊäÈëÊÇµ¥ÐÐ£©
		string line;
		getline(cin, line);
		Json::Value input;
		Json::Reader reader;
		reader.parse(line, input);

		// Ê×ÏÈ´¦ÀíµÚÒ»»ØºÏ£¬µÃÖª×Ô¼ºÊÇË­¡¢ÓÐÄÄÐ©ÅÆ
		{
			auto firstRequest = input["requests"][0u]; // ÏÂ±êÐèÒªÊÇ unsigned£¬¿ÉÒÔÍ¨¹ýÔÚÊý×ÖºóÃæ¼ÓuÀ´×öµ½
			auto own = firstRequest["own"];
			for (unsigned i = 0; i < own.size(); i++)
				myCards.insert(own[i].asInt());
			if (!firstRequest["bid"].isNull())
			{
				// Èç¹û»¹¿ÉÒÔ½Ð·Ö£¬Ôò¼ÇÂ¼½Ð·Ö
				auto bidHistory = firstRequest["bid"];
				myPosition = bidHistory.size();
				for (unsigned i = 0; i < bidHistory.size(); i++)
					bidInput.push_back(bidHistory[i].asInt());
			}
		}

		// historyÀïµÚÒ»Ïî£¨ÉÏÉÏ¼Ò£©ºÍµÚ¶þÏî£¨ÉÏ¼Ò£©·Ö±ðÊÇË­µÄ¾ö²ß
		int whoInHistory[] = {0,0};
		//(myPosition - 1 + PLAYER_COUNT) % PLAYER_COUNT, (myPosition - 2 + PLAYER_COUNT) % PLAYER_COUNT};

		int turn = input["requests"].size();
		for (int i = 0; i < turn; i++)
		{
			auto request = input["requests"][i];
			auto llpublic = request["publiccard"];
			if (!llpublic.isNull())
			{
				// µÚÒ»´ÎµÃÖª¹«¹²ÅÆ¡¢µØÖ÷½Ð·ÖºÍµØÖ÷ÊÇË­
				landlordPosition = request["landlord"].asInt();
				landlordBid = request["finalbid"].asInt();
				myPosition = request["pos"].asInt();
				myPosition=(myPosition-landlordPosition+PLAYER_COUNT)%PLAYER_COUNT;
				cardRemaining[0] += llpublic.size();
				whoInHistory[0] = (myPosition - 2 + PLAYER_COUNT) % PLAYER_COUNT;
				whoInHistory[1] = (myPosition - 1 + PLAYER_COUNT) % PLAYER_COUNT;

				for (unsigned i = 0; i < llpublic.size(); i++)
				{
					landlordPublicCards.insert(llpublic[i].asInt());
					if (0 == myPosition)
						myCards.insert(llpublic[i].asInt());
				}
			}

			auto history = request["history"]; // Ã¿¸öÀúÊ·ÖÐÓÐÉÏ¼ÒºÍÉÏÉÏ¼Ò³öµÄÅÆ
			if (history.isNull())
				continue;
			stage = Stage::PLAYING;

			// Öð´Î»Ö¸´¾ÖÃæµ½µ±Ç°
			int howManyPass = 0;
			for (int p = 0; p < 2; p++)
			{
				int player = whoInHistory[p];	// ÊÇË­³öµÄÅÆ
				auto playerAction = history[p]; // ³öµÄÄÄÐ©ÅÆ
				//cout<<p<<' '<<player<<' '<<myPosition<<endl;
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // Ñ­»·Ã¶¾ÙÕâ¸öÈË³öµÄËùÓÐÅÆ
				{
					int card = playerAction[_].asInt(); // ÕâÀïÊÇ³öµÄÒ»ÕÅÅÆ
					playedCards.push_back(card);
				}
				//cout<<playedCards.size()<<endl;
				whatTheyPlayed[player].push_back(playedCards); // ¼ÇÂ¼Õâ¶ÎÀúÊ·
				cardRemaining[player] -= playerAction.size();

				if (playerAction.size() == 0)
					howManyPass++;
				else
				{ 
					lastValidCombo = CardCombo(playedCards.begin(), playedCards.end());
					lastPosition = player;
				}
			}

			if (howManyPass == 2)
				lastValidCombo = CardCombo(),lastPosition=-1;

			if (i < turn - 1)
			{
				// »¹Òª»Ö¸´×Ô¼ºÔø¾­³ö¹ýµÄÅÆ
				auto playerAction = input["responses"][i]; // ³öµÄÄÄÐ©ÅÆ
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // Ñ­»·Ã¶¾Ù×Ô¼º³öµÄËùÓÐÅÆ
				{
					int card = playerAction[_].asInt(); // ÕâÀïÊÇ×Ô¼º³öµÄÒ»ÕÅÅÆ
					myCards.erase(card);				// ´Ó×Ô¼ºÊÖÅÆÖÐÉ¾µô
					playedCards.push_back(card);
				}
				whatTheyPlayed[myPosition].push_back(playedCards); // ¼ÇÂ¼Õâ¶ÎÀúÊ·
				cardRemaining[myPosition] -= playerAction.size();
			}
		}
	}

	/**
	* Êä³ö½Ð·Ö£¨0, 1, 2, 3 ËÄÖÖÖ®Ò»£©
	*/
	void bid(int value)
	{
		Json::Value result;
		result["response"] = value;

		Json::FastWriter writer;
		cout << writer.write(result) << endl;
	}

	/**
	* Êä³ö´òÅÆ¾ö²ß£¬beginÊÇµü´úÆ÷Æðµã£¬endÊÇµü´úÆ÷ÖÕµã
	* CARD_ITERATORÊÇCard£¨¼´short£©ÀàÐÍµÄµü´úÆ÷
	*/
	template <typename CARD_ITERATOR>
	void play(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		Json::Value result, response(Json::arrayValue);
		for (; begin != end; begin++)
			response.append(*begin);
		result["response"] = response;

		Json::FastWriter writer;
		cout << writer.write(result) << endl;
	}
}


#ifndef _BOTZONE_ONLINE
	template<class T>
	int envaluate(T a,T b){
		set<Card> temp;
		for (;a!=b;a++) temp.insert(*a);
		return Envaluator_inital::envaluate(temp);
	}
	void get_inital_score(){
		vector<int> Score;
		static Card temp[100];
		for (int i=1;i<=54;i++) temp[i]=i-1;
		for (int i=1;i<=100000;i++){
			if (i%20==0) printf("Envaluating situation %d, %d ms passed\n",i,clock());
			random_shuffle(temp+1,temp+55);
			Score.push_back(envaluate(temp+1,temp+18));
		}
		sort(Score.begin(),Score.end());
		freopen("score_2.out","w",stdout);
		for (auto i:Score) printf("%d\n",i); 
	}
#endif

namespace Legal_Move_Set{
	const int min_straight_len[]={
		0,5,3,2
	};//Ë³×Ó×îÐ¡µÄ³¤¶È
	vector<Card> op[20];
	vector<CardCombo> Legal_move;
	//ÕÒËùÓÐ¿ÉÒÔµÄ´øµÄ·½°¸
	void Update(vector<Card> combo,int banl,int banr,int rem,int num,int ind=0){
		if (!rem){
			Legal_move.push_back(CardCombo(combo.begin(),combo.end()));
			return;
		}
		for (;;){
			if (ind==15) break;
			if ((ind<banl||ind>banr)&&op[ind].size()>=num){
				for (int i=0;i<num;i++) combo.push_back(op[ind][i]);
				Update(combo,banl,banr,rem-1,num,ind+1);
				for (int i=0;i<num;i++) combo.pop_back();
			}
			++ind;
		}
	}
	//ÕÒµ½ËùÓÐºÏ·¨µÄ²Ù×÷
	vector<CardCombo> find_legal_move(set<Card> S,CardCombo pre_move){
		for (Level i=0;i<=14;i++)
			op[i].resize(0);
		for (auto i:S)
			op[card2level(i)].push_back(i);
		Legal_move.resize(0);
		
		//cout<<"P2.1"<<endl;
		for (Level i=0;i<=14;i++)
			switch (op[i].size()){
				case 4:
					Legal_move.push_back(CardCombo(op[i].begin(),op[i].begin()+4));//bomb
					Update(vector<Card>(op[i].begin(),op[i].begin()+4),i,i,2,1);//ËÄ+Á½¸ö
					Update(vector<Card>(op[i].begin(),op[i].begin()+4),i,i,2,2);//ËÄ´øÁ½¶Ô
				case 3:
					Legal_move.push_back(CardCombo(op[i].begin(),op[i].begin()+3));//Èý´øÁã
					Update(vector<Card>(op[i].begin(),op[i].begin()+3),i,i,1,1);//Èý´øÒ»
					Update(vector<Card>(op[i].begin(),op[i].begin()+3),i,i,1,2);//Èý´ø¶ø
				case 2:
					Legal_move.push_back(CardCombo(op[i].begin(),op[i].begin()+2));//¶Ô×Ó
				case 1:
					Legal_move.push_back(CardCombo(op[i].begin(),op[i].begin()+1));//µ¥ÕÅ
				case 0:
					break;
			}
		if (op[13].size()&&op[14].size()){
			vector<Card> combo={52,53};
			Legal_move.push_back(CardCombo(combo.begin(),combo.end()));//ÍõÕ¨
		}
		//cout<<"P2.2"<<endl;
		for (int straight_num=1;straight_num<=3;++straight_num)
			for (Level l=0;l<=11-min_straight_len[straight_num]+1;l++){
				bool avaliable=1;
				vector<Card> combo;
				int loc=l;
				for (int i=0;i<min_straight_len[straight_num];i++,loc++)
					if (op[loc].size()>=straight_num)
						for (int j=0;j<straight_num;j++) combo.push_back(op[loc][j]);
					else avaliable=0;
				for (;avaliable;){
					//cout<<l<<' '<<loc<<' '<<straight_num<<endl;
					Legal_move.push_back(CardCombo(combo.begin(),combo.end()));//Ë³×Ó
					if (straight_num==3){
						Update(combo,l,loc-1,loc-l,1);
						Update(combo,l,loc-1,loc-l,2);//·É»ú
					}
					if (loc>11||op[loc].size()<straight_num) break;
					for (int j=0;j<straight_num;j++) combo.push_back(op[loc][j]);
					++loc;
				}
			}
		//cout<<"P2.3"<<endl;
		//for (auto i:Legal_move)
		//	i.debugPrint();
		if (pre_move.comboType==CardComboType::PASS)
			return Legal_move;
		vector<CardCombo> temp=Legal_move;
		Legal_move.resize(0);
		for (auto i:temp)
			if (pre_move.canBeBeatenBy(i))
				Legal_move.push_back(i);
		return Legal_move;
	}
}


namespace Mid_envaluate{
	int card_rem[3][15];
	
	
	int Scoreing(multiset<Card> S){
		return Envaluator_inital::envaluate_ver_temp(S);
	}
	//¸øÒ»¸ö³öÅÆÅÆ×é£¬ÇóËûµÄµÃ·Ö
	int F(int level){
		return (int)(pow(1.37,level));
	}
	int G(int len){
		return max(len*2,(int)(pow(1.58,len)));
	}
	double Comboscore(CardCombo op){
		int score=0;
		int ComboLevel=op.comboLevel;
		int Combolen=op.cards.size();
		if (op.comboType==CardComboType::STRAIGHT2) Combolen/=2;
		if (op.comboType==CardComboType::PLANE) Combolen/=3;
		if (op.comboType==CardComboType::PLANE1) Combolen/=4;
		if (op.comboType==CardComboType::PLANE2) Combolen/=5;
		switch (op.comboType){
			case CardComboType::PASS:
				score=0;
				break;
			case CardComboType::SINGLE:
				score=1+2*F(ComboLevel);
				break;
			case CardComboType::PAIR:
				score=3+3*F(ComboLevel);
				break;
			case CardComboType::STRAIGHT:
				score=1+6*F(ComboLevel)+3*G(Combolen);
				break;
			case CardComboType::STRAIGHT2:
				score=1+6*F(ComboLevel)+5*G(Combolen);
				break;
			case CardComboType::TRIPLET:
				score=8+8*F(ComboLevel);
				break; 
			case CardComboType::TRIPLET1:
				score=12+8*F(ComboLevel);
				break;
			case CardComboType::TRIPLET2:
				score=18+8*F(ComboLevel);
				break;
			case CardComboType::BOMB:
				score=50+20*F(ComboLevel);
				break;
			case CardComboType::QUADRUPLE2:
				score=100+20*F(ComboLevel);
				break;
			case CardComboType::QUADRUPLE4:
				score=150+20*F(ComboLevel);
				break;
			case CardComboType::PLANE:
				score=1+6*F(ComboLevel)+8*G(Combolen);
				break;
			case CardComboType::PLANE1:
				score=1+6*F(ComboLevel)+10*G(Combolen);
				break;
			case CardComboType::PLANE2:
				score=1+6*F(ComboLevel)+12*G(Combolen);
				break;
			case CardComboType::ROCKET:
				score=800;
				break;
			default:
				//ÀíÂÛÉÏ²»»á³öÏÖÔÚÕâÀï
				return 0;
		}
		return score+pow(max(0.1,1.0*(2000-score)),0.7);
	}
	double Score2prob(double score){
		return exp(score/1500.0);
	}//¸ø·ÖÖÎ¹À¼Æ³öÏÖÏà¶Ô¸ÅÂÊ
	double envaluate(multiset<Card> S0,multiset<Card> S1,multiset<Card> S2,CardCombo combo){
		#ifdef zyy
			cout<<"Cards:"<<' '<<S0.size()<<' '<<S1.size()<<' '<<S2.size()<<endl;
			cout<<cardRemaining[0]<<' '<<cardRemaining[1]<<' '<<cardRemaining[2]<<endl;
			for (auto i:S0) cout<<i<<' '; cout<<endl;
			for (auto i:S1) cout<<i<<' '; cout<<endl;
			for (auto i:S2) cout<<i<<' '; cout<<endl;
		#endif
		memset(card_rem,0,sizeof(card_rem));
		for (auto i:S0) ++card_rem[0][i];
		for (auto i:S1) ++card_rem[1][i];
		for (auto i:S2) ++card_rem[2][i];
		
		double score0=Scoreing(S0);
		double score1=Scoreing(S1);
		double score2=Scoreing(S2);
		double score;
		if (myPosition==0) score=score0-max(score1,score2)*0.55-min(score1,score2)*0.5;
		if (myPosition==1) score=score1*0.95+score2*0.05-score0;
		if (myPosition==2) score=score2*0.95+score1*0.05-score0;

		
		#ifdef zyy
			cout<<Comboscore(combo)<<' '<<score<<' '<<score0<<' '<<score1<<' '<<score2<<endl;
		#endif
		if (lastPosition==-1||((lastPosition==0)==(myPosition==0))) score+=0.1*Comboscore(combo);
		else score+=Comboscore(combo); 
		if (myPosition!=0) score*=Score2prob(score0);
		if (myPosition!=1) score*=Score2prob(score1);
		if (myPosition!=2) score*=Score2prob(score2);
		return score;
	}
}
namespace Action{
	int other_remain[20];//³ýÈ¥µØÖ÷ÒÑ¾­³öµôµÄÅÆ£¬Á½¸öÍæ¼Ò×Ü¹²Ê£ÓàµÄÅÆÊý
	set<Card> LordPublicCards;//°ÑµØÖ÷ÒÑ¾­³öµôµÄÅÆ£¬ÒÑÖªµÄÔÚµØÖ÷ÊÖÀïµÄÅÆ
	vector<CardCombo> Valid;
	//Ëæ»úÊý×ÖÉú³ÉÆ÷
	struct MyRandomizer{
		unsigned long long seed;
		MyRandomizer(){
			seed=5437623875342532ull;
		}
		unsigned long long rand(int x){
			seed^=seed<<3;
			seed^=seed>>7;
			seed^=seed<<13;
			return seed;
		}
	}Rnd;
	
	//²Â²âÒ»ÖÖ¿ÉÄÜµÄ¾ÖÃæ
	//·Ö³ÉµØÖ÷/Å©ÃñA/Å©ÃñB
	void Split_Card(multiset<Card> &S0,multiset<Card> &S1,multiset<Card> &S2){
		S0.clear(); S1.clear(); S2.clear();
		vector<Card> temp;
		for (Level i=0;i<=14;i++)
			for (int j=0;j<other_remain[i];j++)
				temp.push_back(i);
		for (int j=1;j<temp.size();j++)
			swap(temp[j],temp[Rnd.rand(0)%(j+1)]);
		int remain;
		switch (myPosition){
			case 0:
				for (auto i:myCards)
					S0.insert(card2level(i));
				remain=cardRemaining[1];
				for (auto i:temp)
					if (remain){
						--remain;
						S1.insert(i);
					}
					else
						S2.insert(i);
				break;
			case 1:
				int used_less;
			case 2:
				for (auto i:myCards)
					S1.insert(card2level(i));
				remain=cardRemaining[0]-LordPublicCards.size();
				for (auto i:temp)
					if (remain){
						--remain;
						S0.insert(i);
					}
					else
						S2.insert(i);
				for (auto i:LordPublicCards)
					S0.insert(card2level(i));
				if (myPosition==2)
					swap(S1,S2);
				break;
		}
	}
	
	//ÕÒ²Ù×÷
	CardCombo findAction(){
		for (int i=0;i<=12;i++) other_remain[i]=4;
		other_remain[13]=other_remain[14]=1;
		for (auto i:myCards) other_remain[card2level(i)]--;
		LordPublicCards=landlordPublicCards;
		//cout<<"P1"<<endl;
		for (int i=0;i<=2;i++)
			for (auto j:whatTheyPlayed[i])
				for (auto k:j){
					other_remain[card2level(k)]--;
					if (LordPublicCards.find(k)!=LordPublicCards.end())
						LordPublicCards.erase(k);
				}
		if (myPosition!=0)//²»ÊÇµØÖ÷
			for (auto i:LordPublicCards)
				other_remain[card2level(i)]--;
			
		//cout<<"P2"<<endl;
		//for (int i=0;i<=14;i++)
		//	cout<<other_remain[i]<<' '; cout<<endl;
		Valid=Legal_Move_Set::find_legal_move(myCards,lastValidCombo);
		//cout<<"P3"<<endl;
		if (Valid.size()==0) return CardCombo(-1,-1);//Òª²»Æð
		if (lastValidCombo.comboType!=CardComboType::PASS)
			Valid.push_back(CardCombo(-1,-1));//¹ý
		//cout<<"P4"<<endl;
		vector<double> score;
		score.resize(Valid.size());
		int play_round=0;
		//#ifdef zyy
		for (;clock()<=0.7*CLOCKS_PER_SEC;){
			multiset<Card> Player0,Player1,Player2;
			Split_Card(Player0,Player1,Player2);
			int index=0;
			for (auto combo:Valid){
				multiset<Card> nPlayer0(Player0);
				multiset<Card> nPlayer1(Player1);
				multiset<Card> nPlayer2(Player2);
				switch (myPosition){
					case 0:
						for (auto i:combo.cards)
							nPlayer0.erase(nPlayer0.find(card2level(i)));
						break;
					case 1:
						for (auto i:combo.cards)
							nPlayer1.erase(nPlayer1.find(card2level(i)));
						break;
					case 2:
						for (auto i:combo.cards)
							nPlayer2.erase(nPlayer2.find(card2level(i)));
						break;
				}
				score[index]+=Mid_envaluate::envaluate(nPlayer0,nPlayer1,nPlayer2,combo);
				//cout<<score[index]<<endl;
				index++;
			}
			play_round++;
			//cerr<<clock()<<endl; 
		}
		int index=0;
		for (auto combo:Valid){
		//	cerr<<score[index]<<" ";
			score[index]+=play_round*combo.cards.size()*50;
		//	cerr<<score[index]<<endl;
			index++;
		}
		#ifdef zyy
			cout<<"Round Played:"<<play_round<<endl;
			for (int i=0;i<score.size();i++){
				Valid[i].debugPrint();
				cout<<"Score: "<<score[i]<<endl;
			}
		#endif
		//cout<<"P6"<<endl;
		//int index=max_element(score.begin(),score.end())-score.begin();
		return Valid[max_element(score.begin(),score.end())-score.begin()];
		/*
		Todo List
		¸øÒ»¸öÖÐÅÌµÄ¾ÖÃæ£¬¼ÆËãÆäµÃ·Ö/¸ø³öÆä¹À¼Û
		*/
	}
}

//½Ð·Ö½×¶ÎµÄ²ÎÊý
//Èç¹ûµÃ·Ö¸ßÓÚconstant[3]½Ð3·Ö,...,ÒÀ´ÎÀàÍÆ
//Õâ²¿·ÖÃ»Ð´
const int constant[4]={-1,-1,-1,-1};

const int constantv[5]={0,1000,2000,3000,0}; 
int main(){
	#ifdef zyy
		freopen("1.in","r",stdin);
	//	freopen("1.out","w",stdout);
	#endif
	/*get_inital_score();*/ 
	srand(time(nullptr));
	BotzoneIO::read();

	if (stage == Stage::BIDDING)
	{
		// cout<<"GGMYFRIEND"<<endl;
		// ×ö³ö¾ö²ß£¨ÄãÖ»ÐèÐÞ¸ÄÒÔÏÂ²¿·Ö£©

		auto maxBidIt = std::max_element(bidInput.begin(), bidInput.end());
		int maxBid = (maxBidIt == bidInput.end() ? -1 : *maxBidIt);
		int bidValue = 0;
		int value = Envaluator_inital::envaluate(myCards);
		
		for (int i=1;i<=3;i++)
			if (value>=constantv[i])
				if (i>maxBid){
					bidValue=i;
				}
		// ¾ö²ß½áÊø£¬Êä³ö½á¹û£¨ÄãÖ»ÐèÐÞ¸ÄÒÔÉÏ²¿·Ö£©

		BotzoneIO::bid(bidValue);
	}
	else if (stage == Stage::PLAYING)
	{
		// ×ö³ö¾ö²ß£¨ÄãÖ»ÐèÐÞ¸ÄÒÔÏÂ²¿·Ö£©
		// findFirstValid º¯Êý¿ÉÒÔÓÃ×÷ÐÞ¸ÄµÄÆðµã
		CardCombo myAction = Action::findAction();
		// ÊÇºÏ·¨ÅÆ
		assert(myAction.comboType != CardComboType::INVALID);
		assert(
			// ÔÚÉÏ¼ÒÃ»¹ýÅÆµÄÊ±ºò¹ýÅÆ
			(lastValidCombo.comboType != CardComboType::PASS && myAction.comboType == CardComboType::PASS) ||
			// ÔÚÉÏ¼ÒÃ»¹ýÅÆµÄÊ±ºò³ö´òµÃ¹ýµÄÅÆ
			(lastValidCombo.comboType != CardComboType::PASS && lastValidCombo.canBeBeatenBy(myAction)) ||
			// ÔÚÉÏ¼Ò¹ýÅÆµÄÊ±ºò³öºÏ·¨ÅÆ
			(lastValidCombo.comboType == CardComboType::PASS && myAction.comboType != CardComboType::INVALID));
		// ¾ö²ß½áÊø£¬Êä³ö½á¹û£¨ÄãÖ»ÐèÐÞ¸ÄÒÔÉÏ²¿·Ö£©
		BotzoneIO::play(myAction.cards.begin(), myAction.cards.end());
	}
	
}
/*
11
6
*/
