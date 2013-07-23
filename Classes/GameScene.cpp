#include "GameScene.h"
#include "SimpleAudioEngine.h"
#include "CCPlaySE.h"

using namespace cocos2d;
using namespace CocosDenshion;
using namespace std;

//初期化
int GameScene::preTouchTag = -1;
int GameScene::postTouchTag = -1;
bool GameScene::addFlag = false;

CCScene* GameScene::scene()
{
    CCScene* scene = CCScene::create();
    GameScene* layer = GameScene::create();
    scene->addChild(layer);
    return scene;
}

// 初期化
bool GameScene::init()
{
    if (!CCLayer::init()) { return false; }
    
    BlockSprite::setGameManager(this);

    // プレイヤーの用意（最大体力、最大スキルポイント、攻撃力、回復力、スキルポイントチャージ力）
    player = new Player(1000, 100, 10, 10, 10);
    
    // 獲得コイン数初期化
    coin = 0;
    
    // コンボ数初期化
    m_combo = 0;
    
    // タップイベントを取得する
    setTouchEnabled(true);
    setTouchMode(kCCTouchesOneByOne);
    
    // アニメーションの登録
    signUpAnimation();
    
    // バックキー・メニューキーイベントを取得する
    setKeypadEnabled(true);
    
    // 変数初期化
    initForVariables();
    
    // 背景表示
    showBackground();
    
    // ブロック表示
    showBlock();
    
    // リセットボタン作成
    showResetButton();
    
    // 効果音の事前読み込み
    SimpleAudioEngine::sharedEngine()->preloadEffect(MP3_REMOVE_BLOCK);
    
    // 4秒後にヒントが出る設定
    unschedule(schedule_selector(GameScene::showSwapChainPosition));
    scheduleOnce(schedule_selector(GameScene::showSwapChainPosition), HINT_TIME);
    
    BlockSprite::setGameManager(this);
    
    return true;
}


// 変数初期化
void GameScene::initForVariables()
{
    // 乱数初期化
    srand((unsigned)time(NULL));
    
#pragma mark BlockSpriteクラス変更に伴う微調整
    // ブロックの一辺の長さを取得
    BlockSprite* pBlock = BlockSprite::createWithBlockType(kBlockPig, 0, 0);
    m_blockSize = pBlock->getContentSize().height;
    
    m_score = 0;
    
    allMoved = true;
}


// 背景表示
void GameScene::showBackground()
{
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    // パズルの背景を生成
    m_background = CCSprite::create(PNG_BACKGROUND);
    addChild(m_background, kZOrderBackground, 0);

    m_background->setPosition(ccp(winSize.width / 2,
                                  m_background->getContentSize().height / 2));
    
    CCSprite *backFrame = CCSprite::create("frame.png");
    m_background->addChild(backFrame, kZOrderBackground);

    backFrame->setPosition(ccp(m_background->getContentSize().width / 2,
                               m_background->getContentSize().height / 2));
    
    const char *score = CCString::createWithFormat("%d", m_score)->getCString();
    CCLabelBMFont *scoreLabel;
    scoreLabel = CCLabelBMFont::create(score, "ui_score_number.fnt");
    scoreLabel->setPosition(ccp(m_background->getContentSize().width - scoreLabel->getContentSize().width / 2, winSize.height - scoreLabel->getContentSize().height));
    m_background->addChild(scoreLabel, kZOrderScore, kTagScoreNumber);
    
}

// ブロック表示
void GameScene::showBlock()
{
    do {
        // 6 x 6 のブロックを作成する
        for (int x = 0; x < MAX_BLOCK_X; x++)
        {
            for (int y = 0; y < MAX_BLOCK_Y; y++)
            {
                // 設置ブロックのタイプの除外リスト
                list<int> matchTypes;
                matchTypes.clear();
                
                // 横の後ろ2つをチェック
                int blockTag1 = kTagBaseBlock + (x - 1) * 100 + y;
                BlockSprite *block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
                int blockTag2 = kTagBaseBlock + (x - 2) * 100 + y;
                BlockSprite *block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
                
                // 2つ同じブロックが並んでいれば、設置ブロックのタイプの除外リストに追加
                if (block1 != NULL &&
                    block2 != NULL &&
                    block1->getBlockType() == block2->getBlockType())
                {
                    matchTypes.push_back(block1->getBlockType());
                }
                
                // 横の後ろ2つをチェック
                blockTag1 = kTagBaseBlock + x * 100 + y - 1;
                block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
                blockTag2 = kTagBaseBlock + x * 100 + y - 2;
                block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
                
                // 2つ同じブロックが並んでいれば、設置ブロックのタイプの除外リストに追加
                if (block1 != NULL &&
                    block2 != NULL &&
                    block1->getBlockType() == block2->getBlockType())
                {
                    matchTypes.push_back(block1->getBlockType());
                }
                
                bool isMatch = true;
                
                kBlock blockType;
                while (isMatch) {
                    isMatch = false;
                    
                    // ランダムでブロックを作成
                    blockType = (kBlock)(rand() % kBlockCount);
                    
                    // 除外リストと比較して、一致したら生成し直し
                    list<int>::iterator it = matchTypes.begin();
                    while( it != matchTypes.end() ) {
                        if(blockType == *it) {
                            isMatch = true;
                        }
                        ++it;
                    }
                }
                
                // 対応するブロック配列にタグを追加
                int tag = getTag(x, y);
                
                // ブロックを作成
                BlockSprite* pBlock = BlockSprite::createWithBlockType(blockType, x, y);
                pBlock->setPosition(getPosition(x, y));
                pBlock->setIndexX(x);
                pBlock->setIndexY(y);
                m_background->addChild(pBlock, kZOrderBlock, tag);
            }
        }
    } while (getSwapChainCount() <= 0);
}


// ヒントをランダムに1つ表示
void GameScene::showSwapChainPosition()
{
    list<BlockTagPair> hintPositions = getSwapChainPositions();
    list<BlockTagPair>::iterator it = hintPositions.begin(); // イテレータ
    
    int setHintPosition = rand() % hintPositions.size();
    int count = 0;
    
    while( it != hintPositions.end() ) {
        BlockTagPair position = *it;
        ++it;  // イテレータを１つ進める
        
        if(setHintPosition == count) {
            BlockSprite::PositionIndex pos1 = getPositionIndex(position.tag1);
            BlockSprite::PositionIndex pos2 = getPositionIndex(position.tag2);
            CCPoint point1 = getPosition(pos1.x, pos1.y);
            CCPoint point2 = getPosition(pos2.x, pos2.y);
            CCPoint setPoint = ccp((point1.x + point2.x) / 2,
                                   (point1.y + point2.y) / 2);
            CCSprite *circle = CCSprite::create("circle.png");
            
            circle->setPosition(setPoint);
            circle->setTag(kTagHintCircle);
            
            m_background->addChild(circle, 100);
            CCRotateBy *actionRoll = CCRotateBy::create(2.0f, 360);
            CCRepeatForever * actionRollForever = CCRepeatForever::create(actionRoll);
            circle->runAction(actionRollForever);
            break;
        }
        
        count++;
    }
}


// 位置取得 (0 <= posIndexX <= 6 , 0 <= posIndexY <= 6)
CCPoint GameScene::getPosition(int posIndexX, int posIndexY)
{
    return CCPoint((posIndexX + 0.5) * m_blockSize, (posIndexY + 0.5) * m_blockSize);
}

// タグ取得 (0 <= posIndexX <= 6 , 0 <= posIndexY <= 6)
int GameScene::getTag(int posIndexX, int posIndexY)
{
    return kTagBaseBlock + posIndexX * 100 + posIndexY;
}

// タッチ開始イベント
bool GameScene::ccTouchBegan(CCTouch* pTouch, CCEvent* pEvent)
{
    CCPoint touchPoint = m_background->convertTouchToNodeSpace(pTouch);
    int tag = 0;
    kBlock blockType;
    getTouchBlockTag(touchPoint, tag, blockType);
        
    //触った場所にブロックがあった場合
    if (tag != 0) {
        BlockSprite *bSprite = (BlockSprite *)m_background->getChildByTag(tag);
        if (bSprite->m_blockState == BlockSprite::kStopping) {
            preTouchTag = tag;
            return true;
        }
    }

    return false;
}

// タッチ移動イベント
void GameScene::ccTouchMoved(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent)
{
    
    CCPoint touchPoint = m_background->convertTouchToNodeSpace(pTouch);
    int tag = 0;
    kBlock blockType;
    getTouchBlockTag(touchPoint, tag, blockType);

    if (!m_isSwappedBlocks && tag != 0) {
        m_isSwappedBlocks = true;
        postTouchTag = tag;
        if (checkCorrectSwap(preTouchTag, postTouchTag)) {
            BlockSprite *beganSprite = (BlockSprite*)m_background->getChildByTag(preTouchTag);
            BlockSprite *movedSprite = (BlockSprite*)m_background->getChildByTag(postTouchTag);
            
            if (beganSprite->m_blockState == BlockSprite::kStopping && movedSprite->m_blockState == BlockSprite::kStopping) {
                beganSprite->setPartnerBlock(movedSprite);
                movedSprite->setPartnerBlock(beganSprite);
                beganSprite->changePosition();
                movedSprite->changePosition();
            }

        } else {
            m_isSwappedBlocks = false;
        }
    }
}

// タッチ終了イベント
void GameScene::ccTouchEnded(CCTouch* pTouch, CCEvent* pEvent)
{
    m_isSwappedBlocks = false;
}

//上下左右に動いたかどうか(正しいスワップがされたかどうか)
bool GameScene::checkCorrectSwap(int preTag, int postTag)
{
    int tags[] = {
        preTouchTag + 100,
        preTouchTag - 100,
        preTouchTag + 1,
        preTouchTag - 1,
    };
    
    for (int i = 0; i < sizeof(tags) / sizeof(tags[0]); i++) {
        if (tags[i] == postTag) {
            return true;
        }
    }
    
    return false;
}

//　与えられたタグのブロックの状態を全てkDeletingに変更する
void GameScene::setDeletingFlags(std::list<int> removeBlockTags) {
    list<int>::iterator it = removeBlockTags.begin();
    while (it != removeBlockTags.end()) {
        BlockSprite *removeSprite = (BlockSprite*)m_background->getChildByTag(*it);
        removeSprite->m_blockState = BlockSprite::kDeleting;
        it++;

    }
}

void GameScene::removeChainBlocks()
{
    // 消滅できるブロックリスト
    list<int> removeChainBlocks;
    
    // 消滅候補ブロックリスト
    list<int> removeReserveBlocks;
    
    // 1行ずつ横の連なりを走査
    for (int y = 0; y <= 5; y++) {
        // 比較対象のブロックの種類
        kBlock currentType;
        
        for (int x = 0; x <= 5; x++) {
            // ターゲットのブロックを取得
            int targetTag = kTagBaseBlock + x * 100 + y;
            BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
            if (target == NULL) {
                continue;
            }
            kBlock targetType = target->getBlockType();
            
            // カレントとターゲットが同じ種類のブロックかどうか
            if (targetType == currentType) {
                // 同じなら消滅候補に追加
                removeReserveBlocks.push_back(targetTag);
                
            } else {
                // 違うならカレントをターゲットに変更
                currentType = targetType;
                
                // その時点で消滅候補が３つ以上（繋がりが３つ以上）なら
                if (removeReserveBlocks.size() >= 3) {
                    list<int>::iterator it = removeReserveBlocks.begin(); // イテレータ
                    while( it != removeReserveBlocks.end() ) {
                        removeChainBlocks.push_back(*it);
                        ++it;  // イテレータを１つ進める
                    }
                }
                
                // 消滅候補を空にして、ターゲットを追加
                removeReserveBlocks.clear();
                removeReserveBlocks.push_back(targetTag);
            }
        }
        
        // 対象の行の走査終了時も消滅候補をチェックして空にする
        if (removeReserveBlocks.size() >= 3) {
            list<int>::iterator it = removeReserveBlocks.begin(); // イテレータ
            while( it != removeReserveBlocks.end() ) {
                removeChainBlocks.push_back(*it);
                ++it;  // イテレータを１つ進める
            }
        }
        
        removeReserveBlocks.clear();
    }
    
    // 1列ずつ縦の連なりを走査
    for (int x = 0; x <= 5; x++) {
        // 比較対象のブロックの種類
        kBlock currentType;
        
        for (int y = 0; y <= 5; y++) {
            // ターゲットのブロックを取得
            int targetTag = kTagBaseBlock + x * 100 + y;
            BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
            kBlock targetType = target->getBlockType();
            
            // カレントとターゲットが同じ種類のブロックかどうか
            if (targetType == currentType) {
                // 同じなら消滅候補に追加
                removeReserveBlocks.push_back(targetTag);
            } else {
                // 違うならカレントをターゲットに変更
                currentType = targetType;
                
                // その時点で消滅候補が３つ以上（繋がりが３つ以上）なら
                if (removeReserveBlocks.size() >= 3) {
                    list<int>::iterator it = removeReserveBlocks.begin(); // イテレータ
                    while( it != removeReserveBlocks.end() ) {
                        removeChainBlocks.push_back(*it);
                        ++it;  // イテレータを１つ進める
                    }
                }
                
                // 消滅候補を空にして、ターゲットを追加
                removeReserveBlocks.clear();
                removeReserveBlocks.push_back(targetTag);
            }
        }
        
        // 対象の行の走査終了時も消滅候補をチェックして空にする
        if (removeReserveBlocks.size() >= 3) {
            list<int>::iterator it = removeReserveBlocks.begin(); // イテレータ
            while( it != removeReserveBlocks.end() ) {
                removeChainBlocks.push_back(*it);
                ++it;  // イテレータを１つ進める
            }
        }
        
        removeReserveBlocks.clear();
    }
    
    removeChainBlocks.sort();
    removeChainBlocks.unique();
    removeChainBlocks.reverse();
    
    removeBlocks(removeChainBlocks);
    /*
    list<int>::iterator it = removeChainBlocks.begin();
    while(it != removeChainBlocks.end()) {
        BlockSprite *removeSprite = (BlockSprite*)m_background->getChildByTag(*it);
        removeSprite->m_blockState = BlockSprite::kDeleting;
        removeSprite->removeSelfAnimation();
        it++;
    }
     */
    
    CCDelayTime *readyDelay = CCDelayTime::create(REMOVING_TIME * 2);
    CCDelayTime *removingDelay = CCDelayTime::create(SWAPPING_TIME * 2);
    CCCallFunc *func1 = CCCallFunc::create(this, callfunc_selector(GameScene::setPreDrop));
    CCCallFunc *func2 = CCCallFunc::create(this, callfunc_selector(GameScene::setNewPosition));
    CCFiniteTimeAction *action = CCSequence::create(readyDelay, func1, removingDelay, func2, NULL);
    this->runAction(action);
}

void GameScene::recursiveCheck() {
    if(!allMoved) {
        list<int> removeList;
        for (int x = 0; x < MAX_BLOCK_X; x++) {
            for (int y = 0; y < MAX_BLOCK_Y; y++) {
                BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(getTag(x, y));
                if (bSprite == NULL) {
                    return;
                }
                list<int> list = checkChain(bSprite);
                if (0 < list.size()) {
                    removeList.merge(list);
                    removeList.sort();
                    removeList.unique();
                }
            }
        }
        
        searchAndSetDeleteType(removeList);
        
        list<int>::iterator it = removeList.begin();
        while (it != removeList.end()) {
            it++;
        }
        
        if (3 <= removeList.size()) {
            removeBlocks(removeList);
            if (GameScene::addFlag) {
                while (!GameScene::addFlag){
                    
                }
                addBlocks();
                m_combo += getRemoveColors(removeList);
                allMoved = true;
            } else {
                addBlocks();
                m_combo += getRemoveColors(removeList);
                allMoved = true;
            }
        }
        
        // 2コンボ以上のときはアニメ演出
        if (m_combo >= 2) {
            showCombo();
        }

        unschedule(schedule_selector(GameScene::resetCombo));
        scheduleOnce(schedule_selector(GameScene::resetCombo), COMBO_TIME);
        
        
        // ヒントサークルが表示されていれば、消去する
        CCNode *circle = m_background->getChildByTag(GameScene::kTagHintCircle);
        if(circle != NULL) {
            circle->removeFromParentAndCleanup(true);
        }

        // ブロックが消えるとき、ヒント表示までの時間をリセットする
        unschedule(schedule_selector(GameScene::showSwapChainPosition));
        scheduleOnce(schedule_selector(GameScene::showSwapChainPosition), HINT_TIME);
    }
}

int GameScene::getRemoveColors(std::list<int> removeBlockTags) {
    list<int>::iterator it = removeBlockTags.begin();
    list<int> colorLists;
    while(it != removeBlockTags.end()) {
        BlockSprite *removeSprite = (BlockSprite*)m_background->getChildByTag(*it);
        colorLists.push_back(removeSprite->getBlockType());
        it++;
    }
    colorLists.sort();
    colorLists.unique();
    return colorLists.size();
}

// 指定されたブロックリストを削除する
void GameScene::removeBlocks(list<int> removeBlockTags)
{
    list<int>::iterator it = removeBlockTags.begin();
    while(it != removeBlockTags.end()) {
        BlockSprite *removeSprite = (BlockSprite*)m_background->getChildByTag(*it);
        if (removeSprite == NULL) {
            it++;
            continue;
        }
        
        removeSprite->m_blockState = BlockSprite::kDeleting;
        removeSprite->removeSelfAnimation();
        it++;
    }
}

// 盤面にブロックを追加する
void GameScene::addBlocks() {
    addFlag = true;
    
    CCDelayTime *readyDelay = CCDelayTime::create(REMOVING_TIME);
    CCDelayTime *removingDelay = CCDelayTime::create(SWAPPING_TIME);
    CCCallFunc *func1 = CCCallFunc::create(this, callfunc_selector(GameScene::setPreDrop));
    CCCallFunc *func2 = CCCallFunc::create(this, callfunc_selector(GameScene::setNewPosition));
    CCFiniteTimeAction *action = CCSequence::create(readyDelay, func1, removingDelay, func2, NULL);
    this->runAction(action);
}

//　渡されたブロックを中心にチェインがあるか走査し、そのリストを返す.
list<int> GameScene::checkChain(BlockSprite *bSprite) {
    list<int> removeBlockTags;
    list<int> removeBlockTagsTemp;
    
    // 上方向に走査
    int count = 0;
    for (int y = bSprite->m_positionIndex.y; y < MAX_BLOCK_Y; y++) {
        int targetTag = getTag(bSprite->m_positionIndex.x, y);
        BlockSprite *targetSprite = (BlockSprite*)m_background->getChildByTag(targetTag);
        if (targetSprite != NULL && bSprite->getBlockType() == targetSprite->getBlockType()) {
            removeBlockTagsTemp.push_back(targetTag);
            count++;
        } else {
            break;
        }
    }
    
    // 下方向に走査
    for (int y = bSprite->m_positionIndex.y - 1; 0 <= y; y--) {
        int targetTag = getTag(bSprite->m_positionIndex.x, y);
        BlockSprite *targetSprite = (BlockSprite*)m_background->getChildByTag(targetTag);
        if (targetSprite != NULL && bSprite->getBlockType() == targetSprite->getBlockType()) {
            removeBlockTagsTemp.push_back(targetTag);
            count++;
        } else {
            break;
        }
    }
    
    if (3 <= count) {
        removeBlockTags.merge(removeBlockTagsTemp);
    }
    
    removeBlockTagsTemp.clear();
    
    // 右方向に走査
    count = 0;
    for (int x = bSprite->m_positionIndex.x; x < MAX_BLOCK_X; x++) {
        int targetTag = getTag(x, bSprite->m_positionIndex.y);
        BlockSprite *targetSprite = (BlockSprite*)m_background->getChildByTag(targetTag);
        if (targetSprite != NULL && bSprite->getBlockType() == targetSprite->getBlockType()) {
            removeBlockTagsTemp.push_back(targetTag);
            count++;
        } else {
            break;
        }
    }

    // 左方向に走査
    for (int x = bSprite->m_positionIndex.x -1; 0 <= x; x--) {
        int targetTag = getTag(x, bSprite->m_positionIndex.y);
        BlockSprite *targetSprite= (BlockSprite*)m_background->getChildByTag(targetTag);
        if (targetSprite != NULL && bSprite->getBlockType() == targetSprite->getBlockType()) {
            removeBlockTagsTemp.push_back(targetTag);
            count++;
        } else {
            break;
        }
    }
    
    if (3 <= count) {
        removeBlockTags.merge(removeBlockTagsTemp);
    }
    
    removeBlockTagsTemp.clear();
    removeBlockTags.sort();
    removeBlockTags.unique();
    removeBlockTags.reverse();
    
    
    list<int>::iterator it = removeBlockTags.begin();
    while (it != removeBlockTags.end()) {
        BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(*it);
        it++;
    }
    
    return removeBlockTags;
}

void GameScene::setDeleteType(std::list<int> removeBlockColorTags) {
    // 一時的に保存するためのリスト
    list<int> chainColorList;
    
    removeBlockColorTags.sort();
    list<int>::iterator it = removeBlockColorTags.begin();
    int baseTag = *it;
    chainColorList.push_back(*it);
    removeBlockColorTags.remove(*it);
    while (it != removeBlockColorTags.end()) {
        if (removeBlockColorTags.size() == 0) {
            break;
        } else {
            it++;
        }

        list<int> mergeList = seekChainRecursive(removeBlockColorTags, baseTag, 0);
        chainColorList.merge(mergeList);
        
        list<int>::iterator it1 = chainColorList.begin();
        bool setLevelFlag = false;
        if (3 == chainColorList.size()) {
            while (it1 != chainColorList.end()) {
                BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(*it1);
                if (bSprite == NULL) {
                    it1++;
                    continue;
                }
                bSprite->deleteState = BlockSprite::kDeleteThree;
                if (bSprite->getPartnerBlock()) {
                    //bSprite->m_blockLevel = 0;
                }
                it1++;
            }
        } else if (4 == chainColorList.size()) {
            while (it1 != chainColorList.end()) {
                BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(*it1);
                if (bSprite == NULL) {
                    it1++;
                    continue;
                }
                bSprite->deleteState = BlockSprite::kDeleteFour;
                //bSprite->m_blockLevel = 0;
                if (bSprite->getPartnerBlock() && !setLevelFlag) {
                    //bSprite->m_blockLevel = 1;
                    bSprite->m_isLevelUp = true;
                    bSprite->setPartnerBlock(NULL);
                    setLevelFlag = true;
                }
                it1++;
            }

            if (!setLevelFlag) {
                it1 = chainColorList.begin();
                BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(*it1);
                //bSprite->m_blockLevel = 1;
                bSprite->m_isLevelUp = true;
            }
        } else if (5 <= chainColorList.size()) {
            while (it1 != chainColorList.end()) {
                BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(*it1);
                if (bSprite == NULL) {
                    it1++;
                    continue;
                }
                bSprite->deleteState = BlockSprite::kDeleteFive;
                //bSprite->m_blockLevel = 0;
                if (bSprite->getPartnerBlock() && !setLevelFlag) {
                    //bSprite->m_blockLevel = 2;
                    bSprite->m_isLevelUp = true;
                    bSprite->setPartnerBlock(NULL);
                    setLevelFlag = true;
                }
                it1++;
            }
            
            if (!setLevelFlag) {
                it1 = chainColorList.begin();
                BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(*it1);
                //bSprite->m_blockLevel = 2;
                bSprite->m_isLevelUp = true;
            }
        } else {
            while (it1 != chainColorList.end()) {
                BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(*it1);
                if (bSprite != NULL) {
                    bSprite->m_blockState = BlockSprite::kStopping;
                    if (bSprite->getPartnerBlock()) {
                        //bSprite->m_blockLevel = 0;
                    }
                }
                it1++;
            }
            
            removeBlockColorTags.clear();
            chainColorList.clear();
            break;
        }
        
        list<int>::iterator itt = chainColorList.begin();
        while (itt != chainColorList.end()) {
            removeBlockColorTags.remove(*itt);
            itt++;
        }

        removeBlockColorTags.clear();
    }
}

list<int> GameScene::seekChainRecursive(list<int> &removeBlockColorTags, int baseTag, int preTag)
{
    list<int> seek;

    if (removeBlockColorTags.size() == 0) {
        return seek;
    }
    
    list<int>::iterator it = removeBlockColorTags.begin();

    while (it != removeBlockColorTags.end()) {
        // 自分自身は操作しない.
        if (baseTag == *it) {
            it++;
            continue;
        }

        if (baseTag + 100 == *it) {
            seek.push_back(*it);
            list<int> checkRightLists = seekChainRecursive(removeBlockColorTags, *it, 0);
            seek.merge(checkRightLists);
        }
        
        if (baseTag + 1 == *it) {
            // 上のブロックが前回検査したものなら行わない
            if (baseTag + 1 != preTag) {
                seek.push_back(*it);
                list<int> checkUpLists = seekChainRecursive(removeBlockColorTags, *it, baseTag);
                seek.merge(checkUpLists);
            }
        }
        
        if (baseTag - 1 == *it) {
            // 下のブロックが前回検査したものなら行わない
            if (baseTag - 1 != preTag) {
                seek.push_back(*it);
                list<int> checkDownLists = seekChainRecursive(removeBlockColorTags, *it, baseTag);
                seek.merge(checkDownLists);
            }
        }
        it++;
    }

    removeBlockColorTags.remove(baseTag);
    
    return seek;
}

void GameScene::updateScore() {
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    const char *score = CCString::createWithFormat("%d", m_score)->getCString();
    CCLabelBMFont *scoreLabel = (CCLabelBMFont*)m_background->getChildByTag(kTagScoreNumber);
    scoreLabel->setCString(score);
    scoreLabel->setPosition(ccp(m_background->getContentSize().width - scoreLabel->getContentSize().width / 2, winSize.height - scoreLabel->getContentSize().height));
    
    //scoreLabel = CCLabelBMFont::create(score, "ui_score_number.fnt");
    /*
    scoreLabel->setPosition(ccp(winSize.width - 30, winSize.height - 50));
    m_background->addChild(scoreLabel, kZOrderScore, kTagScoreNumber);
    */
}

void GameScene::searchAndSetDeleteType(std::list<int> removeBlockTags)
{
    list<int>::iterator tes = removeBlockTags.begin();
    while (tes != removeBlockTags.end()) {
        tes++;
    }
    
    //各色のタグを格納するリスト
    list<int>pigTags;
    list<int>humanRedTags;
    list<int>chickTags;
    list<int>humanWhiteTags;
    
    // 走査
    list<int>::iterator it = removeBlockTags.begin();
    while (it != removeBlockTags.end()) {
        BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(*it);
        switch (bSprite->getBlockType()) {
            case kBlockPig:
                pigTags.push_back(getTag(bSprite->m_positionIndex.x, bSprite->m_positionIndex.y));
                break;
            case kBlockHumanRed:
                humanRedTags.push_back(getTag(bSprite->m_positionIndex.x, bSprite->m_positionIndex.y));
                break;
            case kBlockChick:
                chickTags.push_back(getTag(bSprite->m_positionIndex.x, bSprite->m_positionIndex.y));
                break;
            case kBlockHumanWhite:
                humanWhiteTags.push_back(getTag(bSprite->m_positionIndex.x, bSprite->m_positionIndex.y));
               break;
            default:
                break;
        }
        it++;
    }

    
    if (3 <= pigTags.size()) {
        list<int>::iterator it = pigTags.begin();
        while (it != pigTags.end()) {
            it++;
        }
        setDeleteType(pigTags);
    }
    
    if (3 <= humanRedTags.size()) {
        list<int>::iterator it = humanRedTags.begin();
        while (it != humanRedTags.end()) {
            it++;
        }

        setDeleteType(humanRedTags);
    }
    
    if (3 <= chickTags.size()) {
        list<int>::iterator it = chickTags.begin();
        while (it != chickTags.end()) {
            it++;
        }
        setDeleteType(chickTags);
    }
    
    if (3 <= humanWhiteTags.size()) {
        list<int>::iterator it = humanWhiteTags.begin();
        while (it != humanWhiteTags.end()) {
            it++;
        }
        setDeleteType(humanWhiteTags);
    }
    
}


// 盤面全体を走査し、ブロックの新しいポジションを設定
void GameScene::setNewPosition()
{
    updateScore();
    for (int x = 0; x < MAX_BLOCK_X; x++) {
        int yChain = 0;
        
        for (int y = MAX_BLOCK_Y - 1; 0 <= y; y--) {
            BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(getTag(x, y));
            
            if (bSprite == NULL) {
                kBlock blockType = (kBlock)(rand() % kBlockCount);
                int tag = getTag(x, MAX_BLOCK_Y + yChain);
                BlockSprite *newBlock = BlockSprite::createWithBlockType(blockType, x, MAX_BLOCK_Y + yChain);
                
                // 画面外にブロックを設置
                newBlock->setPosition(getPosition(x, MAX_BLOCK_Y + yChain));
                m_background->addChild(newBlock, kZOrderBlock, tag);
                newBlock->m_prePositionIndex = newBlock->m_positionIndex;
                
                // 一段ずつズラしていく
                for(int dropY = y + 1; dropY < MAX_BLOCK_Y * 2; dropY++) {
                    BlockSprite *dropBlock = (BlockSprite *)m_background->getChildByTag(getTag(x, dropY));
                    if (dropBlock == NULL) {
                        if (dropY < MAX_BLOCK_Y) {
                            continue;
                        }
                        break;
                    }
                    
                    if (dropBlock->m_postPositionIndex.x == -1 || dropBlock->m_postPositionIndex.y == -1) {
                        dropBlock->m_postPositionIndex = dropBlock->m_positionIndex;
                    }
                    int minusIndex = 1;
                    
                    if (MAX_BLOCK_Y < dropBlock->m_postPositionIndex.y) {
                        minusIndex += yChain;
                    }
                    BlockSprite::PositionIndex dPosition = dropBlock->m_postPositionIndex;
                    dPosition.y = dPosition.y - minusIndex;
                    dropBlock->m_prePositionIndex = dropBlock->m_positionIndex;
                    dropBlock->m_postPositionIndex = dPosition;
                    dropBlock->m_blockState = BlockSprite::kPreDropping;
                }
                yChain++;
            } 
        }
    }

    for (int x = 0; x < MAX_BLOCK_X; x++) {
        for (int y = 0; y < MAX_BLOCK_Y * 2; y++) {
            BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(getTag(x, y));
            if (bSprite == NULL) {
                continue;
            }
            
            if (bSprite->m_blockState == BlockSprite::kPreDropping) {
                bSprite->setTag(getTag(bSprite->m_postPositionIndex.x, bSprite->m_postPositionIndex.y));
                bSprite->dropBlock();
            }
        }
    }
    addFlag = false;
}


void GameScene::setPreDrop()
{
    for (int x = 0; x < MAX_BLOCK_X; x++) {
        
        for (int y = MAX_BLOCK_Y - 1; 0 <= y; y--) {
            BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(getTag(x, y));
            
            if (bSprite == NULL) {
                
                // 一段ずつズラしていく
                for(int dropY = y + 1; dropY < MAX_BLOCK_Y * 2; dropY++) {
                    BlockSprite *dropBlock = (BlockSprite *)m_background->getChildByTag(getTag(x, dropY));
                    
                    if (dropBlock == NULL) {
                        if (dropY < MAX_BLOCK_Y) {
                            continue;
                        }
                        break;
                    }
                    
                    if (dropBlock->m_postPositionIndex.x == -1 || dropBlock->m_postPositionIndex.y == -1) {
                        dropBlock->m_postPositionIndex = dropBlock->m_positionIndex;
                    }
                    dropBlock->m_blockState = BlockSprite::kPreDropping;
                }
            }
        }
    }
}


// ヒント（入れ替えで連結）の場所リストを取得
list<GameScene::BlockTagPair> GameScene::getSwapChainPositions()
{
    list<BlockTagPair> swapChainPosition;
    
    for (int x = 0; x < MAX_BLOCK_X; x++) {
        
        for (int y = 0; y < MAX_BLOCK_Y; y++) {
            int blockTag = getTag(x, y);
            BlockSprite *block = (BlockSprite*)m_background->getChildByTag(blockTag);
            
            if(block == NULL) { continue; }
            
            // ブロックの種類
            kBlock blockType = block->getBlockType();
            
            // 間を1つ空けて横の後ろ2つをチェック
            int blockTag1 = kTagBaseBlock + (x - 2) * 100 + y;
            BlockSprite *block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
            int blockTag2 = kTagBaseBlock + (x - 3) * 100 + y;
            BlockSprite *block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
            
            // 一つ空けてターゲットのブロックと同じブロックが二つ並んでいたら
            // 潜在的なブロックと見なす
            if (block1 != NULL &&
                block2 != NULL &&
                block1->getBlockType() == block2->getBlockType() &&
                blockType == block1->getBlockType())
            {
                BlockTagPair position = BlockTagPair(blockTag, kTagBaseBlock + (x - 1) * 100 + y);
                swapChainPosition.push_back(position);
            }
            
            // 間を1つ空けて前の後ろ2つをチェック
            blockTag1 = kTagBaseBlock + (x + 2) * 100 + y;
            block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
            blockTag2 = kTagBaseBlock + (x + 3) * 100 + y;
            block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
            
            // 一つ空けてターゲットのブロックと同じブロックが二つ並んでいたら
            // 潜在的なブロックと見なす
            if (block1 != NULL &&
                block2 != NULL &&
                block1->getBlockType() == block2->getBlockType() &&
                blockType == block1->getBlockType())
            {
                BlockTagPair position = BlockTagPair(blockTag, kTagBaseBlock + (x + 1) * 100 + y);
                swapChainPosition.push_back(position);
            }
            
            // 間を1つ空けて縦の後ろ2つをチェック
            blockTag1 = kTagBaseBlock + x * 100 + y - 2;
            block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
            blockTag2 = kTagBaseBlock + x * 100 + y - 3;
            block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
            
            // 一つ空けてターゲットのブロックと同じブロックが二つ並んでいたら
            // 潜在的なブロックと見なす
            if (block1 != NULL &&
                block2 != NULL &&
                block1->getBlockType() == block2->getBlockType() &&
                blockType == block1->getBlockType())
            {
                BlockTagPair position = BlockTagPair(blockTag, kTagBaseBlock + x * 100 + y - 1);
                swapChainPosition.push_back(position);
            }
            
            // 間を1つ空けて縦の前2つをチェック
            blockTag1 = kTagBaseBlock + x * 100 + y + 2;
            block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
            blockTag2 = kTagBaseBlock + x * 100 + y + 3;
            block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
            
            // 一つ空けてターゲットのブロックと同じブロックが二つ並んでいたら
            // 潜在的なブロックと見なす
            if (block1 != NULL &&
                block2 != NULL &&
                block1->getBlockType() == block2->getBlockType() &&
                blockType == block1->getBlockType())
            {
                BlockTagPair position = BlockTagPair(blockTag, kTagBaseBlock + x * 100 + y + 1);
                swapChainPosition.push_back(position);
            }
            
            int tags[] = {
                1,      // 上
                -1,     // 下
                -100,   // 左
                100     // 右
            };
            
            for (int i = 0; i < sizeof(tags) ; i++) {
                int nextToBlockTag = blockTag + tags[i];
                BlockSprite::PositionIndex nextToBlockIndex = getPositionIndex(nextToBlockTag);
                
                // 縦の走査のとき
                if (tags[i] == 1 || tags[i] == -1) {
                    // 横方向の繋がり
                    int count = 1; // 横につながっている個数
                    // 右方向に走査
                    for (int tx = nextToBlockIndex.x + 1; tx <= nextToBlockIndex.x + 2; tx++) {
                        int targetTag = kTagBaseBlock + tx * 100 + nextToBlockIndex.y;
                        BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
                        if (target == NULL || target->getBlockType() != blockType) {
                            break;
                        }
                        if (targetTag != blockTag) {
                            count++;
                        }
                    }
                    
                    // 左方向に走査
                    for (int tx = nextToBlockIndex.x - 1; tx >= nextToBlockIndex.x - 2; tx--) {
                        int targetTag = kTagBaseBlock + tx * 100 + nextToBlockIndex.y;
                        BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
                        if (target == NULL || target->getBlockType() != blockType) {
                            break;
                        }
                        if (targetTag != blockTag) {
                            count++;
                        }
                    }
                    
                    // 3つ繋がっているか
                    if (count >= 3) {
                        BlockTagPair position = BlockTagPair(blockTag, nextToBlockTag);
                        swapChainPosition.push_back(position);
                    }
                }
                
                
                // 横の走査のとき
                if (tags[i] == 100 || tags[i] == -100) {
                    // 縦方向の繋がり
                    int count = 1; // 縦につながっている個数
                    for (int ty = nextToBlockIndex.y + 1; ty <= nextToBlockIndex.y + 2; ty++) {
                        int targetTag = kTagBaseBlock + nextToBlockIndex.x * 100 + ty;
                        BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
                        if (target == NULL || target->getBlockType() != blockType) {
                            break;
                        }
                        if (targetTag != blockTag) {
                            count++;
                        }
                    }
                    
                    for (int ty = nextToBlockIndex.y - 1; ty >= nextToBlockIndex.y - 2; ty--) {
                        int targetTag = kTagBaseBlock + nextToBlockIndex.x * 100 + ty;
                        BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
                        if (target == NULL || target->getBlockType() != blockType) {
                            break;
                        }
                        if (targetTag != blockTag) {
                            count++;
                        }
                    }
                    
                    // 3つ繋がっているか
                    if (count >= 3) {
                        BlockTagPair position = BlockTagPair(blockTag, nextToBlockTag);
                        swapChainPosition.push_back(position);
                    }
                }
            }
        }
    }
    
    return swapChainPosition;
}

// コンボ数の演出
void GameScene::showCombo()
{
    CCDirector* pDirector = CCDirector::sharedDirector();
    CCPoint origin = pDirector->getVisibleOrigin();
    CCSize visibleSize = pDirector->getVisibleSize();
    
    const char *combo = CCString::createWithFormat("%d", m_combo)->getCString();
    CCLabelBMFont *comboLabel;
    comboLabel = CCLabelBMFont::create(combo, "ui_combo_number.fnt");
    comboLabel->setPosition(ccp(origin.x + comboLabel->getContentSize().width / 2 + m_background->getContentSize().width / 2, origin.y + visibleSize.height / 2 + 100));
    addChild(comboLabel);
    
    CCSprite *comboSprite = CCSprite::create("ui_combo_text.png");
    comboSprite->setPosition(ccp(origin.x - comboLabel->getContentSize().width / 2 + m_background->getContentSize().width / 2 - comboSprite->getContentSize().width / 2 + 15, origin.y + visibleSize.height / 2 + 100));
    m_background->addChild(comboSprite, kZOrderCombo, kTagComboText);
    
    float during = COMBO_TIME;
    CCFadeOut *actionFadeOut = CCFadeOut::create(during);
    CCFadeOut *actionFadeOut1 = CCFadeOut::create(during);
    comboLabel->runAction(actionFadeOut);
    comboSprite->runAction(actionFadeOut1);
    
    comboLabel->scheduleOnce(schedule_selector(CCLabelBMFont::removeFromParent), during);
    comboSprite->scheduleOnce(schedule_selector(CCSprite::removeFromParent), during);
    /*
    char comboText[10];
    sprintf(comboText, "%d COMBO!", m_combo);
    CCLabelTTF *comboLabel = CCLabelTTF::create(comboText, "arial", 60);
    
    // 表示できる画面サイズ取得
    CCDirector* pDirector = CCDirector::sharedDirector();
    CCPoint origin = pDirector->getVisibleOrigin();
    CCSize visibleSize = pDirector->getVisibleSize();
    
    comboLabel->setPosition(ccp(origin.x + comboLabel->getContentSize().width / 2 + 20,
                                origin.y + visibleSize.height / 2 + 100));

    comboLabel->setColor(ccc3(255, 255, 255));
    addChild(comboLabel);
    
    float during = COMBO_TIME;
    CCFadeOut *actionFadeOut = CCFadeOut::create(during);
    comboLabel->runAction(actionFadeOut);
    
    comboLabel->unschedule(schedule_selector(CCLabelTTF::removeFromParent));
    comboLabel->scheduleOnce(schedule_selector(CCLabelTTF::removeFromParent), during);
    */
}

// コンボ数のリセット
void GameScene::resetCombo()
{
    m_combo = 0;
}

// ブロックのインデックス取得
BlockSprite::PositionIndex GameScene::getPositionIndex(int tag)
{
    int pos1_x = (tag - kTagBaseBlock) / 100;
    int pos1_y = (tag - kTagBaseBlock) % 100;
    
    return BlockSprite::PositionIndex(pos1_x, pos1_y);
}


/*
 * アニメーションキャッシュの追加
 *   fileName  ファイル名の雛形 xxxx_%01d.png のように連番画像ファイル名の雛形を指定する
 *   cacheName キャッシュに登録する名前　この名前で取り出しが出来る
 *   startNum  画像ファイルの連番開始数値
 *   endNum    画像ファイルの連番終了数値
 *   isReverse trueにすると 画像ファイルのアニメーションを頭から最後に最後から頭までの逆再生を加える
 *   duration  全フレームを表示する秒数の指定
 */
void GameScene::addAnimationCache(const char *fileName, const char *cacheName, int startNum, int endNum , bool isReverse, float duration)
{
    // アニメーションキャッシュはシングルトン
    CCAnimationCache *animationCache = CCAnimationCache::sharedAnimationCache();
    
    // アニメーションフレームを管理するクラス
    CCAnimation *animation = CCAnimation::create();
    
    // アニメーションのコマ分繰り返す
    for (int i=startNum; i<=endNum; i++) {
        // ファイル名を生成
        char szImageFileName[128] = {0};
        sprintf(szImageFileName, fileName, i);
        
        // アニメーション　フレームに画像を追加
        animation->addSpriteFrameWithFileName(szImageFileName);
    }
    
    // 用意した画像を反対の順番で格納する
    if (isReverse) {
        for (int i = endNum; i>=startNum; i--) {
            // ファイル名を生成
            char szImageFileName[128] = {0};
            sprintf(szImageFileName, fileName, i);
            
            // アニメーション　フレームに画像を追加
            animation->addSpriteFrameWithFileName(szImageFileName);
        }
    }
    
    // 引数の"duration"秒間の間で全フレームを表示
    int frameCnt = (animation->getFrames())->count();
    animation->setDelayPerUnit(duration / frameCnt);
    
    // 全フレーム表示後に最初のフレームにもどる設定
    animation->setRestoreOriginalFrame(true);
    
    // 出来たアニメーションをキャッシュに登録
    animationCache->addAnimation(animation, cacheName);
}


// アニメーションの登録
void GameScene::signUpAnimation()
{
    addAnimationCache( "a14_01_%04d@2x.png", "normal", 1, 22, false, 0.2f );
}


// アニメーション取得
CCSprite* GameScene::getAnimation(char* animName)
{
    CCSprite* sprite = CCSprite::create();
    
    CCAnimationCache *animationCache = CCAnimationCache::sharedAnimationCache();
    CCAnimation *animation = animationCache->animationByName(animName);
    CCAnimate* animate = CCAnimate::create(animation);
    
    CCCallFunc* removeSelf = CCCallFunc::create(sprite, callfunc_selector(CCSprite::removeFromParent));
    
    CCSequence* action = CCSequence::create(animate, removeSelf, NULL);
    sprite->runAction(action);

    return sprite;
}


// 指定したブロックに潜在的な連結があるかどうか
int GameScene::getSwapChainBlockCount(int blockTag)
{
    int chainCount = 0;

    BlockSprite *block = (BlockSprite *)m_background->getChildByTag(blockTag);
    
    // ブロックの種類
    kBlock blockType = block->getBlockType();
    // ブロックの盤面上の座標
    int x = getPositionIndex(blockTag).x;
    int y = getPositionIndex(blockTag).y;
    
    // 間を1つ空けて横の後ろ2つをチェック
    int blockTag1 = kTagBaseBlock + (x - 2) * 100 + y;
    BlockSprite *block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
    int blockTag2 = kTagBaseBlock + (x - 3) * 100 + y;
    BlockSprite *block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
    
    // 一つ空けてターゲットのブロックと同じブロックが二つ並んでいたら
    // 潜在的なブロックと見なす
    if (block1 != NULL &&
        block2 != NULL &&
        block1->getBlockType() == block2->getBlockType() &&
        blockType == block1->getBlockType())
    {
        chainCount++;
    }
    
    // 間を1つ空けて前の後ろ2つをチェック
    blockTag1 = kTagBaseBlock + (x + 2) * 100 + y;
    block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
    blockTag2 = kTagBaseBlock + (x + 3) * 100 + y;
    block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
    
    // 一つ空けてターゲットのブロックと同じブロックが二つ並んでいたら
    // 潜在的なブロックと見なす
    if (block1 != NULL &&
        block2 != NULL &&
        block1->getBlockType() == block2->getBlockType() &&
        blockType == block1->getBlockType())
    {
        chainCount++;
    }
    
    // 間を1つ空けて縦の後ろ2つをチェック
    blockTag1 = kTagBaseBlock + x * 100 + y - 2;
    block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
    blockTag2 = kTagBaseBlock + x * 100 + y - 3;
    block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
    
    // 一つ空けてターゲットのブロックと同じブロックが二つ並んでいたら
    // 潜在的なブロックと見なす
    if (block1 != NULL &&
        block2 != NULL &&
        block1->getBlockType() == block2->getBlockType() &&
        blockType == block1->getBlockType())
    {
        chainCount++;
    }
    
    // 間を1つ空けて縦の前2つをチェック
    blockTag1 = kTagBaseBlock + x * 100 + y + 2;
    block1 = (BlockSprite *)m_background->getChildByTag(blockTag1);
    blockTag2 = kTagBaseBlock + x * 100 + y + 3;
    block2 = (BlockSprite *)m_background->getChildByTag(blockTag2);
    
    // 一つ空けてターゲットのブロックと同じブロックが二つ並んでいたら
    // 潜在的なブロックと見なす
    if (block1 != NULL &&
        block2 != NULL &&
        block1->getBlockType() == block2->getBlockType() &&
        blockType == block1->getBlockType())
    {
        chainCount++;
    }
    
    int tags[] = {
        1,      // 上
        -1,     // 下
        -100,   // 左
        100     // 右
    };
    
    for (int i = 0; i < sizeof(tags) ; i++) {
        int nextToBlockTag = blockTag + tags[i];
        BlockSprite::PositionIndex nextToBlockIndex = getPositionIndex(nextToBlockTag);

        // 縦の走査のとき
        if (tags[i] == 1 || tags[i] == -1) {
            // 横方向の繋がり
            int count = 1; // 横につながっている個数
            // 右方向に走査
            for (int tx = nextToBlockIndex.x + 1; tx <= nextToBlockIndex.x + 2; tx++) {
                int targetTag = kTagBaseBlock + tx * 100 + nextToBlockIndex.y;
                BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
                if (target == NULL || target->getBlockType() != blockType) {
                    break;
                }
                if (targetTag != blockTag) {
                    count++;
                }
            }
            
            // 左方向に走査
            for (int tx = nextToBlockIndex.x - 1; tx >= nextToBlockIndex.x - 2; tx--) {
                int targetTag = kTagBaseBlock + tx * 100 + nextToBlockIndex.y;
                BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
                if (target == NULL || target->getBlockType() != blockType) {
                    break;
                }
                if (targetTag != blockTag) {
                    count++;
                }
            }
            
            // 3つ繋がっているか
            if (count >= 3) { chainCount++; }
        }

        
        // 横の走査のとき
        if (tags[i] == 100 || tags[i] == -100) {
            // 縦方向の繋がり
            int count = 1; // 縦につながっている個数
            for (int ty = nextToBlockIndex.y + 1; ty <= nextToBlockIndex.y + 2; ty++) {
                int targetTag = kTagBaseBlock + nextToBlockIndex.x * 100 + ty;
                BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
                if (target == NULL || target->getBlockType() != blockType) {
                    break;
                }
                if (targetTag != blockTag) {
                    count++;
                }
            }
            
            for (int ty = nextToBlockIndex.y - 1; ty >= nextToBlockIndex.y - 2; ty--) {
                int targetTag = kTagBaseBlock + nextToBlockIndex.x * 100 + ty;
                BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
                if (target == NULL || target->getBlockType() != blockType) {
                    break;
                }
                if (targetTag != blockTag) {
                    count++;
                }
            }
            
            // 3つ繋がっているか
            if (count >= 3) { chainCount++; }
        }
    }
    
    return chainCount;
}


// 潜在的な連結の数を取得する
int GameScene::getSwapChainCount()
{
    int chainCount = 0;
    
    for (int x = 0; x < MAX_BLOCK_X; x++) {
        for (int y = 0; y < MAX_BLOCK_Y; y++) {
            int blockTag = kTagBaseBlock + x * 100 + y;
            chainCount += getSwapChainBlockCount(blockTag);
        }
    }
    
    return chainCount;
}


// タップされたブロックのタグを取得
void GameScene::getTouchBlockTag(CCPoint touchPoint, int &tag, kBlock &blockType)
{
    for (int x = 0; x < MAX_BLOCK_X; x++)
    {
        for (int y = 0; y < MAX_BLOCK_Y; y++)
        {
            int currentTag = getTag(x, y);
            CCNode* node = m_background->getChildByTag(currentTag);
            if (node && node->boundingBox().containsPoint(touchPoint))
            {
                tag = currentTag;
                blockType = ((BlockSprite*)node)->getBlockType();
                
                return;
            }
        }
    }
}


// リセットボタンタップ時の処理
void GameScene::menuResetCallback(cocos2d::CCObject* pSender)
{
    GameScene* scene = GameScene::create();
    CCDirector::sharedDirector()->replaceScene((CCScene*)scene);
}


// リセットボタン作成
void GameScene::showResetButton()
{
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    
    // リセットボタン作成
    CCMenuItemImage* resetButton = CCMenuItemImage::create(PNG_RESET, PNG_RESET, this, menu_selector(GameScene::menuResetCallback));
    resetButton->setPosition(ccp(resetButton->getContentSize().width / 2,
                                 winSize.height - resetButton->getContentSize().height / 2));

    // メニュー作成
    CCMenu* menu = CCMenu::create(resetButton, NULL);
    menu->setPosition(CCPointZero);
    addChild(menu, 30000);
}

// Androidバックキーイベント
void GameScene::keyBackClicked()
{
    CCDirector::sharedDirector()->end();
}

// Androidメニューキーイベント
void GameScene::keyMenuClicked()
{
    menuResetCallback(NULL);
}
