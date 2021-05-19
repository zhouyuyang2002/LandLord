#include <iostream>
#include <ctime> 
#include <set>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <cassert>
#include <cstring> // 注意memset是cstring里的
#include <algorithm>
#include "jsoncpp/json.h" // 在平台上，C++编译时默认包含此库


using namespace std;


using std::set;
using std::sort;
using std::string;
using std::unique;
using std::vector;

constexpr int PLAYER_COUNT = 3;

enum class Stage
{
	BIDDING, // 叫分阶段
	PLAYING	 // 打牌阶段
};

enum class CardComboType
{
	PASS,		// 过
	SINGLE,		// 单张
	PAIR,		// 对子
	STRAIGHT,	// 顺子
	STRAIGHT2,	// 双顺
	TRIPLET,	// 三条
	TRIPLET1,	// 三带一
	TRIPLET2,	// 三带二
	BOMB,		// 炸弹
	QUADRUPLE2, // 四带二（只）
	QUADRUPLE4, // 四带二（对）
	PLANE,		// 飞机
	PLANE1,		// 飞机带小翼
	PLANE2,		// 飞机带大翼
	SSHUTTLE,	// 航天飞机
	SSHUTTLE2,	// 航天飞机带小翼
	SSHUTTLE4,	// 航天飞机带大翼
	ROCKET,		// 火箭
	INVALID		// 非法牌型
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

// 用0~53这54个整数表示唯一的一张牌
using Card = short;
constexpr Card card_joker = 52;
constexpr Card card_JOKER = 53;

// 除了用0~53这54个整数表示唯一的牌，
// 这里还用另一种序号表示牌的大小（不管花色），以便比较，称作等级（Level）
// 对应关系如下：
// 3 4 5 6 7 8 9 10	J Q K	A	2	小王	大王
// 0 1 2 3 4 5 6 7	8 9 10	11	12	13	14
using Level = short;
constexpr Level MAX_LEVEL = 15;
constexpr Level MAX_STRAIGHT_LEVEL = 11;
constexpr Level level_joker = 13;
constexpr Level level_JOKER = 14;

/**
* 将Card变成Level
*/
constexpr Level card2level(Card card){
	return card / 4 + card / 53;
}

// 牌的组合，用于计算牌型
struct CardCombo
{
	// 表示同等级的牌有多少张
	// 会按个数从大到小、等级从大到小排序
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
	vector<Card> cards;		 // 原始的牌，未排序
	vector<CardPack> packs;	 // 按数目和大小排序的牌种
	CardComboType comboType; // 算出的牌型
	Level comboLevel = 0;	 // 算出的大小序

	/**
						  * 检查个数最多的CardPack递减了几个
						  */
	int findMaxSeq() const
	{
		for (unsigned c = 1; c < packs.size(); c++)
			if (packs[c].count != packs[0].count ||
				packs[c].level != packs[c - 1].level - 1)
				return c;
		return packs.size();
	}

	// 创建一个空牌组
	CardCombo() : comboType(CardComboType::PASS) {}

	/**
	* 通过Card（即short）类型的迭代器创建一个牌型
	* 并计算出牌型和大小序等
	* 假设输入没有重复数字（即重复的Card）
	*/
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		// 特判：空
		if (begin == end)
		{
			comboType = CardComboType::PASS;
			return;
		}

		// 每种牌有多少个
		short counts[MAX_LEVEL + 1] = {};

		// 同种牌的张数（有多少个单张、对子、三条、四条）
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

		// 用最多的那种牌总是可以比较大小的
		comboLevel = packs[0].level;

		// 计算牌型
		// 按照 同种牌的张数 有几种 进行分类
		vector<int> kindOfCountOfCount;
		for (int i = 0; i <= 4; i++)
			if (countOfCount[i])
				kindOfCountOfCount.push_back(i);
		sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

		int curr, lesser;

		switch (kindOfCountOfCount.size())
		{
		case 1: // 只有一类牌
			curr = countOfCount[kindOfCountOfCount[0]];
			switch (kindOfCountOfCount[0])
			{
			case 1:
				// 只有若干单张
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
				// 只有若干对子
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
				// 只有若干三条
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
				// 只有若干四条
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
		case 2: // 有两类牌
			curr = countOfCount[kindOfCountOfCount[1]];
			lesser = countOfCount[kindOfCountOfCount[0]];
			if (kindOfCountOfCount[1] == 3)
			{
				// 三条带？
				if (kindOfCountOfCount[0] == 1)
				{
					// 三带一
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
					// 三带二
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
				// 四条带？
				if (kindOfCountOfCount[0] == 1)
				{
					// 四条带两只 * n
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
					// 四条带两对 * n
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
	* 判断指定牌组能否大过当前牌组（这个函数不考虑过牌的情况！）
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
		std::cout << "【" << cardComboStrings[(int)comboType] << "共" << cards.size() << "张，大小序" << comboLevel << "】";
#endif
	}
};

/* 状态 */
// 我的牌有哪些
set<Card> myCards;
// 地主明示的牌有哪些
set<Card> landlordPublicCards;
// 大家从最开始到现在都出过什么
vector<vector<Card>> whatTheyPlayed[PLAYER_COUNT];
// 当前要出的牌需要大过谁
CardCombo lastValidCombo;
// 大家还剩多少牌
short cardRemaining[PLAYER_COUNT] = {17, 17, 17};
// 我是几号玩家（0-地主，1-农民甲，2-农民乙）
int myPosition;
// 地主位置
int landlordPosition = -1;
// 地主叫分
int landlordBid = -1;
// 阶段
Stage stage = Stage::BIDDING;

// 自己的第一回合收到的叫分决策
vector<int> bidInput;

//这部分用于叫分时刻的决策。
namespace Envaluator_inital{
	
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
	};//顺子最小的长度
	
	int F(int level){
		return (int)(pow(1.37,level));
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
				temp.push_back(1+6*F(i.ComboLevel)+(3<<i.Combolen));
			if (i.ComboType==CardComboType::STRAIGHT2)
				temp.push_back(1+6*F(i.ComboLevel)+(5<<(2*i.Combolen)));
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
				temp.push_back(1+6*F(i.ComboLevel)+(8<<(2*i.Combolen)));
			if (i.ComboType==CardComboType::PLANE1)
				temp.push_back(1+i.Combolen*20+6*i.ComboLevel+(8<<(2*i.Combolen)));
			if (i.ComboType==CardComboType::PLANE2)
				temp.push_back(1+i.Combolen*60+6*i.ComboLevel+(8<<(2*i.Combolen)));
			if (i.ComboType==CardComboType::ROCKET)
				temp.push_back(800);
		}
		int result=-(1<<temp.size()); /*出牌次数过多有惩罚*/
		for (auto i:temp) result+=i-Punish[i];
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
	
	//贪心处理三带一，三带二的情况
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
			else{
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
	//这里我们认为我们不会特别的欧皇，选到航天飞机 这里选择不判。
	//提取出来没有straight的牌型号
	//这里我们认为不会拆掉形如 222 的牌型
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
		//王炸
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
	//飞机翅膀
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
	//搜出来所有的顺子
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
						//顺子
						case 1:
							Combos.push_back((MyCardCombo){CardComboType::STRAIGHT,straight_ind,loc-straight_ind});
							dfs_straight(straight_num,straight_ind,Combos);
							Combos.pop_back();
							break;
						//连对
						case 2:
							Combos.push_back((MyCardCombo){CardComboType::STRAIGHT2,straight_ind,loc-straight_ind});
							dfs_straight(straight_num,straight_ind,Combos);
							Combos.pop_back();
							break;
						//飞机
						case 3:
							Combos.push_back((MyCardCombo){CardComboType::PLANE,straight_ind,loc-straight_ind});
							dfs_straight(straight_num,straight_ind,Combos);
							Combos.pop_back();
						  //飞机小翅膀
						  	Combos.push_back((MyCardCombo){CardComboType::PLANE1,straight_ind,loc-straight_ind});
						  	dfs_plane(straight_num,straight_ind,1,loc-straight_ind,0,Combos);
						  	Combos.pop_back();
						  //飞机大翅膀
						  	Combos.push_back((MyCardCombo){CardComboType::PLANE2,straight_ind,loc-straight_ind});
						  	dfs_plane(straight_num,straight_ind,2,loc-straight_ind,0,Combos);
						  	Combos.pop_back();
						  	break;
					}
					if (loc>MAX_STRAIGHT_LEVEL||level_num[loc]<=straight_num)
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
		for (int i=1;i<=50000;i++) Punish[i]=(int)(80/sqrt(i));
		//for (auto i:S) cout<<card2level(i)<<endl;
		vector<MyCardCombo> Combos;
		memset(level_num,0,sizeof(level_num));
		for (auto i:S) level_num[card2level(i)]+=1;
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
		// 读入输入（平台上的输入是单行）
		string line;
		getline(cin, line);
		Json::Value input;
		Json::Reader reader;
		reader.parse(line, input);

		// 首先处理第一回合，得知自己是谁、有哪些牌
		{
			auto firstRequest = input["requests"][0u]; // 下标需要是 unsigned，可以通过在数字后面加u来做到
			auto own = firstRequest["own"];
			for (unsigned i = 0; i < own.size(); i++)
				myCards.insert(own[i].asInt());
			if (!firstRequest["bid"].isNull())
			{
				// 如果还可以叫分，则记录叫分
				auto bidHistory = firstRequest["bid"];
				myPosition = bidHistory.size();
				for (unsigned i = 0; i < bidHistory.size(); i++)
					bidInput.push_back(bidHistory[i].asInt());
			}
		}

		// history里第一项（上上家）和第二项（上家）分别是谁的决策
		int whoInHistory[] = {(myPosition - 2 + PLAYER_COUNT) % PLAYER_COUNT, (myPosition - 1 + PLAYER_COUNT) % PLAYER_COUNT};

		int turn = input["requests"].size();
		for (int i = 0; i < turn; i++)
		{
			auto request = input["requests"][i];
			auto llpublic = request["publiccard"];
			if (!llpublic.isNull())
			{
				// 第一次得知公共牌、地主叫分和地主是谁
				landlordPosition = request["landlord"].asInt();
				landlordBid = request["finalbid"].asInt();
				myPosition = request["pos"].asInt();
				cardRemaining[landlordPosition] += llpublic.size();
				for (unsigned i = 0; i < llpublic.size(); i++)
				{
					landlordPublicCards.insert(llpublic[i].asInt());
					if (landlordPosition == myPosition)
						myCards.insert(llpublic[i].asInt());
				}
			}

			auto history = request["history"]; // 每个历史中有上家和上上家出的牌
			if (history.isNull())
				continue;
			stage = Stage::PLAYING;

			// 逐次恢复局面到当前
			int howManyPass = 0;
			for (int p = 0; p < 2; p++)
			{
				int player = whoInHistory[p];	// 是谁出的牌
				auto playerAction = history[p]; // 出的哪些牌
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // 循环枚举这个人出的所有牌
				{
					int card = playerAction[_].asInt(); // 这里是出的一张牌
					playedCards.push_back(card);
				}
				whatTheyPlayed[player].push_back(playedCards); // 记录这段历史
				cardRemaining[player] -= playerAction.size();

				if (playerAction.size() == 0)
					howManyPass++;
				else
					lastValidCombo = CardCombo(playedCards.begin(), playedCards.end());
			}

			if (howManyPass == 2)
				lastValidCombo = CardCombo();

			if (i < turn - 1)
			{
				// 还要恢复自己曾经出过的牌
				auto playerAction = input["responses"][i]; // 出的哪些牌
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // 循环枚举自己出的所有牌
				{
					int card = playerAction[_].asInt(); // 这里是自己出的一张牌
					myCards.erase(card);				// 从自己手牌中删掉
					playedCards.push_back(card);
				}
				whatTheyPlayed[myPosition].push_back(playedCards); // 记录这段历史
				cardRemaining[myPosition] -= playerAction.size();
			}
		}
	}

	/**
	* 输出叫分（0, 1, 2, 3 四种之一）
	*/
	void bid(int value)
	{
		Json::Value result;
		result["response"] = value;

		Json::FastWriter writer;
		cout << writer.write(result) << endl;
	}

	/**
	* 输出打牌决策，begin是迭代器起点，end是迭代器终点
	* CARD_ITERATOR是Card（即short）类型的迭代器
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
	};//顺子最小的长度
	vector<Card> op[20];
	vector<CardCombo> Legal_move;
	//找所有可以的带的方案
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
	//找到所有合法的操作
	vector<CardCombo> find_legal_move(set<Card> S,CardCombo pre_move){
		for (Level i=0;i<=14;i++)
			op[i].resize(0);
		for (auto i:S)
			op[card2level(i)].push_back(i);
		Legal_move.resize(0);
		for (Level i=0;i<=14;i++)
			switch (op[i].size()){
				case 4:
					Legal_move.push_back(CardCombo(op[i].begin(),op[i].begin()+4));//bomb
					Update(vector<Card>(op[i].begin(),op[i].begin()+4),i,i,2,1);//四+两个
					Update(vector<Card>(op[i].begin(),op[i].begin()+4),i,i,2,2);//四带两对
				case 3:
					Legal_move.push_back(CardCombo(op[i].begin(),op[i].begin()+3));//三带零
					Update(vector<Card>(op[i].begin(),op[i].begin()+3),i,i,1,1);//三带一
					Update(vector<Card>(op[i].begin(),op[i].begin()+3),i,i,1,2);//三带而
				case 2:
					Legal_move.push_back(CardCombo(op[i].begin(),op[i].begin()+2));//对子
				case 1:
					Legal_move.push_back(CardCombo(op[i].begin(),op[i].begin()+1));//单张
				case 0:
					break;
			}
		if (op[13].size()&&op[14].size()){
			vector<Card> combo={52,53};
			Legal_move.push_back(CardCombo(combo.begin(),combo.end()));//王炸
		}
		for (int straight_num=1;straight_num<=3;++straight_num)
			for (Level l=0;l<=11-min_straight_len[straight_num]+1;l++){
				bool avaliable=1;
				vector<Card> combo;
				int loc=l;
				for (int i=0;i<min_straight_len[straight_num];i++)
					if (op[loc].size()>=straight_num)
						for (int j=0;j<straight_num;j++) combo.push_back(op[loc][j]);
					else avaliable=0;
				for (;avaliable;){
					
					Legal_move.push_back(CardCombo(combo.begin(),combo.end()));//顺子
					if (straight_num==3){
						Update(combo,l,loc-1,loc-l,1);
						Update(combo,l,loc-1,loc-l,2);//飞机
					}
					if (loc>11||op[loc].size()<straight_num) break;
					++loc;
					for (int j=0;j<straight_num;j++) combo.push_back(op[loc][j]);
				}
			}
		if (pre_move.comboType==CardComboType::PASS)
			return Legal_move;
		vector<CardCombo> temp=Legal_move;
		Legal_move.resize(0);
		for (auto i:Legal_move)
			if (pre_move.canBeBeatenBy(i))
				Legal_move.push_back(i);
		return Legal_move;
	}
}


namespace Mid_envaluate{
	int card_rem[3][15];
	void envaluate(multiset<Card> S0,multiset<Card> S1,multiset<Card> S2){
		memset(card_rem,0,sizeof(card_rem));
		for (auto i:S0) ++card_rem[0][i];
		for (auto i:S1) ++card_rem[1][i];
		for (auto i:S2) ++card_rem[2][i];
		
	}
}
namespace Action{
	int other_remain[20];//除去地主已经出掉的牌，两个玩家总共剩余的牌数
	set<Card> LordPublicCards;//把地主已经出掉的牌，已知的在地主手里的牌
	vector<CardCombo> Valid;
	//随机数字生成器
	struct MyRandomizer{
		unsigned long long seed;
		MyRandomizer(){
			seed=54376238753425325432ull;
		}
		unsigned long long operator (int)(){
			seed^=seed<<3;
			seed^=seed>>7;
			seed^=seed<<11;
			return seed;
		}
	}Rnd;
	
	//猜测一种可能的局面
	//分成地主/农民A/农民B
	void Split_Card(multiset<Card> &S0,multiset<Card> &S1,multiet<Card> &S2){
		S0.clear(); S1.clear(); S2.clear();
		vector<Card> temp;
		for (Level i=0;i<=14;i++)
			for (int j=0;j<other_remain[i];j++)
				temp.push_back(j);
		for (int j=1;j<temp.size();j++)
			swap(temp[j],temp[Rnd(0)%(j+1)]);
		switch (myPosition){
			case:0
				for (auto i:myCards)
					S0.insert(card2level(i));
				int remain=cardRemain[1];
				for (auto i:temp)
					if (remain){
						--remain;
						S1.insert(i);
					}
					else
						S2.insert(i);
				break;
			case:1
			case:2
				for (auto i:myCards)
					S1.insert(card2level(i));
				int remain=cardRemain[2];
				for (auto i:temp)
					if (remain){
						--remain;
						S2.insert(i);
					}
					else
						S0.insert(i);
				for (auto i:LordPublicCards)
					S0.insert(card2level(i));
				if (myPosition==2)
					swap(S1,S2);
				break;
		}
	}
	
	//找操作
	CardCombo findAction(){
		for (int i=0;i<=12;i++) other_remain[i]=4;
		other_remain[13]=other_remain[14]=1;
		for (auto i:myCards) other_remain[card2level(i)]--;
		LordPublicCards=landlordPublicCards;
		for (int i=0;i<=2;i++)
			for (auto j:whatTheyPlayed[i])
				for (auto k:j){
					other_remain[card2level(k)]--;
					if (LordPublicCards.find(k)!=LordPublicCards.end())
						LordPublicCards.erase(k);
				}
		if (myPosition!=0)//不是地主
			for (auto i:LordPublicCards)
				other_remain[card2level(i)]--;
			
		Valid=Legal_Move_Set::find_legal_move(myCards,lastValidCombo);
		if (Valid.size()==0) return CardCombo(-1,-1);//要不起
		if (lastValidCombo.comboType!=CardComboType::PASS)
			Valid.push_back(CardCombo(-1,-1));//过
		
		vector<double> score;
		score.resize(Valid.size());
		for (;clock()<=0.95*CLOCKS_PER_SEC;){
			multiset<Card> Player0,Player1,Player2
			Split(Player0,Player1,Player2);
			int index=0;
			for (auto combo:valid){
				multiset<Card> nPlayer0(Player0);
				multiset<Card> nPlayer1(Player1);
				multiset<Card> nPlayer2(Player2);
				switch (myPosition){
					case 0:
						for (auto i:combo.cards)
							Player0.erase(Player.find(card2level(i)));
						break;
					case 1:
						for (auto i:combo.cards)
							Player1.erase(Player.find(card2level(i)));
						break;
					case 2:
						for (auto i:combo.cards)
							Player2.erase(Player.find(card2level(i)));
						break;
				}
				score[index]+=Mid_envaluate::envaluate(Player0,Player1,Player2);
			}
		}
		
		int index=max_element(score.begin(),score.end())-score.begin();
		return Valid[index];
		/*
		Todo List
		给一个中盘的局面，计算其得分/给出其估价
		*/
	}
}

//叫分阶段的参数
//如果得分高于constant[3]叫3分,...,依次类推
//这部分没写
const int constant[4]={-1,-1,-1,-1};

int main(){
	get_inital_score();
	/*srand(time(nullptr));
	BotzoneIO::read();

	if (stage == Stage::BIDDING)
	{
		// 做出决策（你只需修改以下部分）

		auto maxBidIt = std::max_element(bidInput.begin(), bidInput.end());
		int maxBid = (maxBidIt == bidInput.end() ? -1 : *maxBidIt);
		int bidValue = 0;
		int value = Envaluator_inital::envaluate(MyCards);
		
		for (int i=1;i<=3;i++)
			if (value>=constant[i])
				if (i>maxBidid){
					bidValue=i;
				}
		// 决策结束，输出结果（你只需修改以上部分）

		BotzoneIO::bid(bidValue);
	}
	else if (stage == Stage::PLAYING)
	{
		// 做出决策（你只需修改以下部分）
		// findFirstValid 函数可以用作修改的起点
		CardCombo myAction = Action::findAction();
		// 是合法牌
		assert(myAction.comboType != CardComboType::INVALID);
		assert(
			// 在上家没过牌的时候过牌
			(lastValidCombo.comboType != CardComboType::PASS && myAction.comboType == CardComboType::PASS) ||
			// 在上家没过牌的时候出打得过的牌
			(lastValidCombo.comboType != CardComboType::PASS && lastValidCombo.canBeBeatenBy(myAction)) ||
			// 在上家过牌的时候出合法牌
			(lastValidCombo.comboType == CardComboType::PASS && myAction.comboType != CardComboType::INVALID));
		// 决策结束，输出结果（你只需修改以上部分）
		BotzoneIO::play(myAction.cards.begin(), myAction.cards.end());
	}*/
	
}
/*
11
6
*/