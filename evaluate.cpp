#include<bits/stdc++.h>
#include "jsoncpp/json.h"
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
};

Json::Value getResponse()
{
	system("landlord1");
	Json::Value input;
	Json::Reader reader;
	string line;
	freopen("1.out","r",stdin);
	getline(cin,line);
	fclose(stdin);
	reader.parse(line,input);
	return input;
}

void Output(Json::Value i)
{
	freopen("1.in","w",stdout);
	Json::FastWriter writer;
	cout<<writer.write(i)<<endl;
	fclose(stdout);
}

Json::Value makeJson(set<int> cards)
{
	Json::Value C(Json::arrayValue);
	for (int i:cards) C.append(i);
	return C;
}

Json::Value makeJson(CardCombo cards)
{
	Json::Value C(Json::arrayValue);
	for (int i:cards.cards) C.append(i);
	return C;
}

int main()
{
	srand(time(NULL));
	for (int T=1; ; T++)
	{
		int ini[55],bid[10],score;
		Json::Value result[3];
		set<int> card[3];
		int turn[3],cnt[3];
		for (int i=0; i<3; i++) card[i].clear(),cnt[i]=0;
		for (int i=0; i<54; i++) ini[i]=i;
		random_shuffle(ini,ini+54),score=1;
		for (int i=0; i<3; i++)
		{
			Json::Value Own(Json::arrayValue),Bid(Json::arrayValue);
			for (int j=i*17; j<(i+1)*17; j++) Own.append(ini[j]),card[i].insert(ini[j]);
			for (int j=0; j<i; j++) Bid.append(bid[j]);
			result[i]["requests"][0]["own"]=Own,result[i]["requests"][0]["bid"]=Bid;
			Output(result[i]);
			system("landlord1");
			bid[i]=getResponse()["response"].asInt();
			result[i]["responses"][0]=bid[i];
			turn[i]=0;
		}
		int lord=0;
		for (int i=1; i<3; i++) if (bid[i]>bid[lord]) lord=i;
		score=max(score,bid[lord]);
		for (int i=51; i<54; i++) card[lord].insert(ini[i]);
		int now=lord;
		CardCombo La1,La2;
		while (1)
		{
			Json::Value history(Json::arrayValue);
			history.append(makeJson(La2)),history.append(makeJson(La1));
			result[now]["requests"][++turn[now]]["history"]=history;
			if (turn[now]==1)
			{
				Json::Value publicCard(Json::arrayValue);
				for (int i=51; i<54; i++) publicCard.append(ini[i]);
				result[now]["requests"][turn[now]]["publiccard"]=publicCard;
				result[now]["requests"][turn[now]]["own"]=makeJson(card[now]);
				result[now]["requests"][turn[now]]["landlord"]=lord;
				result[now]["requests"][turn[now]]["pos"]=now;
				result[now]["requests"][turn[now]]["finalbid"]=score;
			}
			Output(result[now]);
			auto playcards=getResponse()["response"];
			result[now]["responses"][turn[now]]=playcards;
			//Json::FastWriter writer;
			//cerr<<playcards.size()<<endl;
			//cerr<<writer.write(result[now])<<endl;
			vector<int> playCards;
			for (int i=0,sz=playcards.size(); i<sz; i++)
				card[now].erase(playcards[i].asInt()),playCards.push_back(playcards[i].asInt());
			CardCombo Nw=CardCombo(playCards.begin(),playCards.end());
			assert(Nw.comboType!=CardComboType::INVALID);
			if (La1.comboType==CardComboType::PASS)
			{
				if (La2.comboType==CardComboType::PASS) assert(Nw.comboType!=CardComboType::PASS);
				else assert(Nw.comboType==CardComboType::PASS||La2.canBeBeatenBy(Nw));
			} else assert(Nw.comboType==CardComboType::PASS||La1.canBeBeatenBy(Nw));
			if (Nw.comboType!=CardComboType::PASS) cnt[now]++;
			if (Nw.comboType==CardComboType::BOMB) score*=2;
			if (Nw.comboType==CardComboType::ROCKET) score*=2;
			if (card[now].empty()) 
			{
				if (now==lord)
				{
					bool bo=0;
					for (int i=0; i<3; i++)
						if (i!=now&&cnt[i]) bo=1;
					if (!bo) score*=2; 
				} else if (cnt[lord]==1) score*=2;
				cerr<<"Case #"<<T<<": ";
				for (int i=0; i<3; i++) 
				{
					int sc=((i==lord)==(now==lord))?score:-score;
					if (i==lord) sc*=2;
					cerr<<sc<<" ";
				}
				cerr<<endl;
				break;
			}
			La2=La1,La1=Nw;
			
			Json::FastWriter writer;
			cerr<<writer.write(result[now])<<endl;
			//Output(result[now]);
			now=(now+1)%3;
			for (int i=0; i<3; i++,cerr<<endl)
			{
				cerr<<i<<": ";
				for (int j:card[i]) cerr<<j<<" ";
			}
			cerr<<endl;
		}
		
	}
	return 0;
}
