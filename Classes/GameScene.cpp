#include "GameScene.h"
#include "SimpleAudioEngine.h"
#include "CCPlaySE.h"

using namespace cocos2d;
using namespace CocosDenshion;
using namespace std;

//初期化
int GameScene::preTouchTag = -1;
int GameScene::postTouchTag = -1;

list<int> GameScene::removeBlockTagLists;
list<int> GameScene::swapBlockTagLists;

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

    // プレイヤーの用意（最大体力、最大スキルポイント、攻撃力、回復力、スキルポイントチャージ力）
    player = new Player(1000, 100, 10, 10, 10);
    
    // 獲得コイン数初期化
    coin = 0;
    
    // タップイベントを取得する
    setTouchEnabled(true);
    setTouchMode(kCCTouchesOneByOne);
    
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
    scheduleOnce(schedule_selector(GameScene::showSwapChainPosition), HINT_TIME);
    
    return true;
}


// 変数初期化
void GameScene::initForVariables()
{
    // 乱数初期化
    srand((unsigned)time(NULL));
    
    // ブロックの一辺の長さを取得
    BlockSprite* pBlock = BlockSprite::createWithBlockType(kBlockRed);
    m_blockSize = pBlock->getContentSize().height;
    
    m_score = 0;
    isChainFlag = false;
}


// 背景表示
void GameScene::showBackground()
{
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    
    // 背景の生成
    CCSprite *background = CCSprite::create("back_ground.png");
    addChild(background);
    background->setPosition(ccp(winSize.width / 2,
                                winSize.height / 2));
    
    // プレイヤーの体力ゲージ生成
    addChild(player->hpGauge, 30000);
    player->hpGauge->setPosition(ccp(winSize.width / 4,
                                     winSize.height - (player->hpGauge->getContentSize().height / 2)));
    
    // 相手プレイヤーの体力ゲージ生成
    CCSprite *enemyHpGauge = CCSprite::create("gauge.png");
    addChild(enemyHpGauge, 30000);
    enemyHpGauge->setPosition(ccp(winSize.width * 3 / 4,
                                  winSize.height - (player->hpGauge->getContentSize().height / 2)));
    
    CCSprite *enemyHpGaugeBar = CCSprite::create("gauge_red_bar.png");
    enemyHpGauge->addChild(enemyHpGaugeBar, 0);
    enemyHpGaugeBar->setPosition(ccp(enemyHpGauge->getContentSize().width / 2,
                                  enemyHpGauge->getContentSize().height / 2));
    
    CCSprite *enemyHpGaugeFrame = CCSprite::create("gauge_frame.png");
    enemyHpGauge->addChild(enemyHpGaugeFrame, 1);
    enemyHpGaugeFrame->setPosition(ccp(enemyHpGauge->getContentSize().width / 2,
                                       enemyHpGauge->getContentSize().height / 2));
    
    // パズルの背景を生成
    m_background = CCSprite::create(PNG_BACKGROUND);
    addChild(m_background, kZOrderBackground, kTagBackground);
    m_background->setPosition(ccp(m_background->getContentSize().width / 2 + 3,
                                  winSize.height - player->hpGauge->getContentSize().height - m_background->getContentSize().height / 2));
    
    CCSprite *backFrame = CCSprite::create("frame.png");
    m_background->addChild(backFrame, 30000);
    backFrame->setPosition(ccp(m_background->getContentSize().width / 2,
                               m_background->getContentSize().height / 2));
    
    // 敵パズルの背景を生成
    CCSprite *enemyBackGround = CCSprite::create(PNG_BACKGROUND);
    addChild(enemyBackGround, kZOrderBackground, kTagBackground);
    float enemyPuzzleScale = 2.0f / 3.5f;
    enemyBackGround->setScale(enemyPuzzleScale);
    enemyBackGround->setPosition(ccp(m_background->getContentSize().width + (winSize.width - m_background->getContentSize().width) / 2 + 3,
                                     winSize.height - enemyHpGauge->getContentSize().height - enemyBackGround->getContentSize().height * enemyPuzzleScale / 2 - 3));
    
    CCSprite *enemyBackFrame = CCSprite::create("frame.png");
    enemyBackGround->addChild(enemyBackFrame, 30000);
    enemyBackFrame->setPosition(ccp(enemyBackGround->getContentSize().width / 2,
                                    enemyBackGround->getContentSize().height / 2));
    
    // スキルフレームの生成
    CCSprite *magicFrame = CCSprite::create("magic_frame.png");
    addChild(magicFrame, 30000);
    magicFrame->setPosition(ccp(m_background->getPositionX() + m_background->getContentSize().width / 2 + magicFrame->getContentSize().width / 2 + 3,
                                enemyBackGround->getPositionY() - enemyBackGround->getContentSize().height * enemyPuzzleScale / 2 - magicFrame->getContentSize().height / 2 - 3));
    
    // スキルアイコンの生成
    CCSprite *magicItem = CCSprite::create("magic02.png");
    magicFrame->addChild(magicItem);
    magicItem->setPosition(ccp(magicFrame->getContentSize().width / 2,
                               magicFrame->getContentSize().height / 2));
    
    // スキルゲージの生成
    addChild(player->magicGauge);
    float magicScale = 2.0f / 3.0f;
    player->magicGauge->setScale(magicScale);
    player->magicGauge->setPosition(ccp(magicFrame->getPositionX() + player->magicGauge->getContentSize().width * magicScale / 2,
                                        magicFrame->getPositionY()));
    
    // コイン表示領域
    CCSprite *coinLabel = CCSprite::create("coin_label.png");
    addChild(coinLabel);
    coinLabel->setPosition(ccp(m_background->getPositionX() + m_background->getContentSize().width / 2 + coinLabel->getContentSize().width / 2,
                               m_background->getPositionY() - m_background->getContentSize().height / 2 + coinLabel->getContentSize().height / 2));
    
    CCSprite *coin = CCSprite::create("coin.png");
    coinLabel->addChild(coin);
    coin->setPosition(ccp(coin->getContentSize().width,
                          coinLabel->getContentSize().height / 2));
    
    coinCount = CCLabelTTF::create("0", "arial", 45);
    coinLabel->addChild(coinCount);
    coinCount->setPosition(ccp(coin->getPositionX() + coin->getContentSize().width / 2 + 30,
                               coinLabel->getContentSize().height / 2));
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
                BlockSprite* pBlock = BlockSprite::createWithBlockType(blockType);
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
            PositionIndex pos1 = getPositionIndex(position.tag1);
            PositionIndex pos2 = getPositionIndex(position.tag2);
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
        if (bSprite->getIsTouchFlag()) {
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
            unschedule(schedule_selector(GameScene::showSwapChainPosition));
            // コンボの初期化
            m_combo = 0;
            BlockSprite *preSprite = (BlockSprite *)m_background->getChildByTag(preTouchTag);
            BlockSprite *postSprite = (BlockSprite *)m_background->getChildByTag(postTouchTag);
            if (preSprite->getIsTouchFlag() && postSprite->getIsTouchFlag()){
                //事前にパートナーを登録
                preSprite->setSwapPartnerTag(preTouchTag);
                postSprite->setSwapPartnerTag(postTouchTag);
                swapBlockTagLists.push_back(postTouchTag);
                swapSprite(preSprite, postSprite);
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

// 2つのスプライトを入れ替える
void GameScene::swapSprite(BlockSprite *swapSprite1, BlockSprite *
                           swapSprite2)
{
    swapSprite1->setIsTouchFlag(false);
    swapSprite2->setIsTouchFlag(false);
    
    // 各々のスプライトの次のスワップ場所を取得する.
    int swapTag1 = getTag(swapSprite1->getIndexX(), swapSprite1->getIndexY());
    int swapTag2 = getTag(swapSprite2->getIndexX(), swapSprite2->getIndexY());
        
    PositionIndex movedSwapPos1 = getPositionIndex(swapTag2);
    PositionIndex movedSwapPos2 = getPositionIndex(swapTag1);
        
    CCPoint swapPos1 = getPosition(movedSwapPos1.x, movedSwapPos1.y);
    CCPoint swapPos2 = getPosition(movedSwapPos2.x, movedSwapPos2.y);
    
    CCMoveBy *swapSpriteMove1 = CCMoveBy::create(MOVING_TIME, ccp(swapPos1.x - swapPos2.x, swapPos1.y - swapPos2.y));
    CCMoveBy *swapSpriteMove2 = CCMoveBy::create(MOVING_TIME, ccp(swapPos2.x - swapPos1.x, swapPos2.y - swapPos1.y));
    
    //CCDelayTime *delay = CCDelayTime::create(MOVING_TIME);
    
    CCCallFuncN *func = CCCallFuncN::create(this, callfuncN_selector(GameScene::swapAnimationFinished));
    CCFiniteTimeAction *action1 = CCSequence::create(swapSpriteMove1, func, NULL);
    //CCFiniteTimeAction *action2 = CCSequence::create(swapSpriteMove2, func, NULL);
    
    swapSprite1->runAction(action1);
    //swapSprite2->runAction(action2);
    swapSprite2->runAction(swapSpriteMove2);
    
    // インデックスの入れ替え
    swapSprite1->setIndexX(movedSwapPos1.x);
    swapSprite1->setIndexY(movedSwapPos1.y);
    swapSprite2->setIndexX(movedSwapPos2.x);
    swapSprite2->setIndexY(movedSwapPos2.y);
    
    // タグの入れ替え
    swapSprite1->setTag(swapTag2);
    swapSprite2->setTag(swapTag1);
    
}

// 入れ替えアニメーションの終了
void GameScene::swapAnimationFinished(BlockSprite *bSprite)
{
    // タッチ可能状態に遷移
    bSprite->setIsTouchFlag(true);

    // パートナーの登録されていなければ終了
    if (bSprite->getSwapPartnerTag() == -1) {
        return;
    }

    BlockSprite *partnerSprite = (BlockSprite*)m_background->getChildByTag(bSprite->getSwapPartnerTag());
    partnerSprite->setIsTouchFlag(true);
    
    isChainFlag = false;
    checkAndRemoveAndDrop();
}

// 削除できるブロックがあれば、removeAndDropを呼び出す
void GameScene::checkAndRemoveAndDrop()
{
    list<int>::iterator it = swapBlockTagLists.begin();
    int swapTag = -1;
    while(it != swapBlockTagLists.end()) {
        swapTag = *it;
        break;
    }

    list<int> removeBlockTags = getRemoveChainBlocks(swapTag);

    // 消えることのできるブロックがある
    if(removeBlockTags.size() >= 3) {
        list<int>::iterator it = swapBlockTagLists.begin();
        // スワップしたブロックのタグを取得するイテレータ
        while(it != swapBlockTagLists.end() ){
            BlockSprite *swapSprite1 = (BlockSprite*)m_background->getChildByTag(*it);
            BlockSprite *swapSprite2;
            if(swapSprite1->getSwapPartnerTag() != -1) {
                swapSprite2 = (BlockSprite*)m_background->getChildByTag(swapSprite1->getSwapPartnerTag());
            }
            list<int>::iterator it1 = removeBlockTags.begin();
            while (it1 != removeBlockTags.end()) {
                // 消すリストに入っている
                if (*it1 == *it) {
                    swapSprite1->setSwapPartnerTag(-1);
                    swapSprite1->setIsTouchFlag(false);
                    swapSprite2->setSwapPartnerTag(-1);
                    swapBlockTagLists.remove(*it);
                }
                // ペアが消すリストに入っている
                else if(*it1 == swapSprite1->getSwapPartnerTag()) {
                    swapSprite2->setSwapPartnerTag(-1);
                    swapSprite2->setIsTouchFlag(false);
                    swapSprite1->setSwapPartnerTag(-1);
                    swapBlockTagLists.remove(*it1);
                }
                it1++;
            }
            it++;
        }

        // 直前にスワイプしたもので、消すリストに入っていなかったものをスワイプブロックタグリストから削除
        list<int>::iterator it1 = swapBlockTagLists.begin();
        while (it1 != swapBlockTagLists.end()) {
            BlockSprite *swapSprite = (BlockSprite*)m_background->getChildByTag(*it1);
            if (swapSprite->getSwapPartnerTag() == -1 && swapSprite->getIsTouchFlag()) {
                swapBlockTagLists.remove(*it1);
            }
                       
            it1++;
        }
        
        // 連結を作ったらヒントを消す
        CCNode *circle = m_background->getChildByTag(kTagHintCircle);
        if (circle != NULL) {
            circle->removeFromParent();
        }

        m_combo++;
        
        // 2コンボ以上のときはアニメ演出
        if (m_combo >= 2) {
            char comboText[10];
            sprintf(comboText, "%d COMBO!", m_combo);
            CCLabelTTF *comboLabel = CCLabelTTF::create(comboText, "arial", 60);
            
            // 表示できる画面サイズ取得
            CCDirector* pDirector = CCDirector::sharedDirector();
            CCPoint origin = pDirector->getVisibleOrigin();
            CCSize visibleSize = pDirector->getVisibleSize();
            
            comboLabel->setPosition(ccp(origin.x + visibleSize.width / 2,
                                        origin.y + visibleSize.height / 2));
            comboLabel->setColor(ccc3(255, 255, 255));
            addChild(comboLabel);
            
            float during = 0.5f;
            CCFadeOut *actionFadeOut = CCFadeOut::create(during);
            CCScaleTo *actionScaleUp = CCScaleTo::create(during, 1.5f);
            comboLabel->runAction(actionFadeOut);
            comboLabel->runAction(actionScaleUp);
            
            comboLabel->scheduleOnce(schedule_selector(CCLabelTTF::removeFromParent), during);
        }
        
        removeBlockTagLists = removeBlockTags;
        
#pragma mark 得点加算は不要かも
        // 得点加算 (消したブロック数 - 2) の2 乗
        m_score += pow(removeBlockTags.size() - 2, 2);

        removeBlocksAniamtion(removeBlockTags, REMOVING_TIME);
        
        scheduleOnce(schedule_selector(GameScene::removeAndDrop), REMOVING_TIME);
    } else {
        // CCLOG("潜在連結数 : %d", getSwapChainCount());
        // 潜在的な連結がないとき
        if (getSwapChainCount() <= 0) {
            #pragma mark TODO: 盤面を新しく用意する
            #pragma mark TODO: ブロックドロップ終了時に移行?
//            swapBlockTagLists.clear();
            CCLOG("No Match!");
        }
        
        if (0 < swapBlockTagLists.size()) {
            list<int>::iterator it = swapBlockTagLists.begin();
            // スワップしたブロック履歴のイテレータ
            while (it != swapBlockTagLists.end())
            {
                BlockSprite *swapSprite1 = (BlockSprite*)m_background->getChildByTag(*it);
                if (swapSprite1->getSwapPartnerTag() != -1) {
                    BlockSprite *swapSprite2 = (BlockSprite*)m_background->getChildByTag(swapSprite1->getSwapPartnerTag());
                    // 1発目の移動アニメーションが終了していれば、戻るアニメーションを実行する
                    if ( swapSprite1->getIsTouchFlag() && swapSprite2->getIsTouchFlag() ) {
                        // パートナーを削除
                        swapSprite1->setSwapPartnerTag(-1);
                        swapSprite2->setSwapPartnerTag(-1);
                        
                        // 元の位置に戻す
                        swapSprite(swapSprite1, swapSprite2);

                        // 移動終了後、動かせるようにする.
                        CCDelayTime *delay = CCDelayTime::create(MOVING_TIME);
                        CCCallFuncN *func = CCCallFuncN::create(this, callfuncN_selector(GameScene::swapAnimationFinished));
                        CCFiniteTimeAction *action1 = CCSequence::create(delay, func, NULL);
                        CCFiniteTimeAction *action2 = CCSequence::create(delay, func, NULL);
                        swapSprite1->runAction(action1);
                        swapSprite2->runAction(action2);

                        // スワップ履歴から削除
                        swapBlockTagLists.remove(swapSprite1->getSwapPartnerTag());
                        swapBlockTagLists.remove(*it);
                    }
                }
                it++;
            }
            
        }
        
        unschedule(schedule_selector(GameScene::showSwapChainPosition));
        scheduleOnce(schedule_selector(GameScene::showSwapChainPosition), HINT_TIME);
    }
}


// 連結していて消滅できるブロックの、タグ配列を取得
list<int> GameScene::getRemoveChainBlocks(int tag) {
    // 消滅できるブロックリスト
    list<int> removeChainBlocks;

#pragma mark swapAnimationFinishedに移動
    if(!isChainFlag) {
        if (tag == -1) {
            return removeChainBlocks;
        }
        
        BlockSprite *bSprite = (BlockSprite*)m_background->getChildByTag(tag);
        if(tag != -1 && bSprite->getSwapPartnerTag() != -1) {
            // 移動させたブロックが連結になったか
            if (! isChainedBlock(tag) &&
                ! isChainedBlock(bSprite->getSwapPartnerTag()))
            {
                // 連結がなければ消えるブロックなし
                return removeChainBlocks;
            }
        }
    }
    
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
    
    
    return removeChainBlocks;
}

// 指定したブロックを含む３つ以上のブロック連結があるかどうか
bool GameScene::isChainedBlock(int blockTag)
{
    BlockSprite *block = (BlockSprite *)m_background->getChildByTag(blockTag);
    
    // ブロックの種類
    kBlock blockType = block->getBlockType();
    // ブロックの盤面上の座標
    PositionIndex blockIndex = getPositionIndex(blockTag);
    
    // 横方向の繋がり
    int count = 1; // 横につながっている個数
    // 右方向に走査
    for (int x = blockIndex.x + 1 ; x < MAX_BLOCK_X; x++) {
        int targetTag = kTagBaseBlock + x * 100 + blockIndex.y;
        BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
        if (target == NULL || target->getBlockType() != blockType) {
            break;
        }
        count++;
    }
    
    // 左方向に走査
    for (int x = blockIndex.x - 1; x >= 0; x--) {
        int targetTag = kTagBaseBlock + x * 100 + blockIndex.y;
        BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
        if (target == NULL || target->getBlockType() != blockType) {
            break;
        }
        count++;
    }
    // 3つ繋がっているか
    if (count >= 3) { return true; }
    
    
    // 縦方向の繋がり
    count = 1; // 縦につながっている個数
    for (int y = blockIndex.y + 1; y < MAX_BLOCK_Y; y++) {
        int targetTag = kTagBaseBlock + blockIndex.x * 100 + y;
        BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
        if (target == NULL || target->getBlockType() != blockType) {
            break;
        }
        count++;
    }
    
    for (int y = blockIndex.y - 1; y >= 0; y--) {
        int targetTag = kTagBaseBlock + blockIndex.x * 100 + y;
        BlockSprite *target = (BlockSprite *)m_background->getChildByTag(targetTag);
        if (target == NULL || target->getBlockType() != blockType) {
            break;
        }
        count++;
    }
    // 3つ繋がっているか
    if (count >= 3) { return true; }
    
    return false;  // 3マッチがない
}

// 配列のブロックの消えるアニメーションを実行
void GameScene::removeBlocksAniamtion(list<int> blockTags, float during)
{
    bool first = true;
    
    list<int>::iterator it = blockTags.begin();
    while (it != blockTags.end())
    {
        // 対象となるブロックを取得
        CCNode* block = m_background->getChildByTag(*it);
        if (block)
        {
            // ブロックが消えるアニメーションを生成
            CCScaleTo* scale = CCScaleTo::create(during, 0);
            
            CCFiniteTimeAction* action;
            if (first)
            {
                // ブロックが消えるサウンドアクションを生成
                CCPlaySE* playSe = CCPlaySE::create(MP3_REMOVE_BLOCK);
                
                // アクションをつなげる
                action = CCSpawn::create(scale, playSe, NULL);
                
                first = false;
            }
            else
            {
                action = scale;
            }
            
            // アクションをセットする
            block->runAction(action);
        }
        
        // これから消えるブロックを触らせない
        BlockSprite *blockSprite = (BlockSprite*)m_background->getChildByTag(*it);
        blockSprite->setIsTouchFlag(false);
        
        it++;
    }
    
    SimpleAudioEngine::sharedEngine()->playEffect(MP3_REMOVE_BLOCK);
}

// ヒント（入れ替えで連結）の場所リストを取得
list<GameScene::BlockTagPair> GameScene::getSwapChainPositions()
{
    list<BlockTagPair> swapChainPosition;
    
    for (int x = 0; x < MAX_BLOCK_X; x++) {
        for (int y = 0; y < MAX_BLOCK_Y; y++) {
            int blockTag = getTag(x, y);
            BlockSprite *block = (BlockSprite*)m_background->getChildByTag(blockTag);
            
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
                PositionIndex nextToBlockIndex = getPositionIndex(nextToBlockTag);
                
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

// 消滅リスト内のブロックを消して、上のブロックを落とすアニメーションセット
void GameScene::removeAndDrop()
{
    // 隣接するブロックを削除する
    removeBlock(removeBlockTagLists);
    
    // ブロック削除後のアニメーション
    movingBlocksAnimation(removeBlockTagLists);
    
    removeBlockTagLists.clear();
}

// 配列のブロックを削除
void GameScene::removeBlock(list<int> blockTags)
{
    int blueCount = 0;
    int grayCount = 0;
    int greenCount = 0;
    int redCount = 0;
    int yellowCount = 0;
    
    list<int>::iterator it = blockTags.begin();
    while (it != blockTags.end())
    {
        // 対象となるブロックを取得
        BlockSprite* block = (BlockSprite *)m_background->getChildByTag(*it);
        if (block)
        {
            // 各種類のブロックの数を数える
            switch (block->getBlockType()) {
                case kBlockBlue:    blueCount++; break;
                case kBlockGray:    grayCount++; break;
                case kBlockGreen:   greenCount++; break;
                case kBlockRed:     redCount++; break;
                case kBlockYellow:  yellowCount++; break;
                default: break;
            }
            // コマの削除
            block->removeFromParentAndCleanup(true);
        }
        it++;
    }
    
    // 消した分だけブロックの効果発動
    if (blueCount >= 1) player->chargeMagicPoint(blueCount);
    if (grayCount >= 1) CCLOG("手錠 %d 個", grayCount);
    if (greenCount >= 1) player->heal(greenCount);
    if( redCount >= 1) player->attack(redCount);
    if (yellowCount >= 1) {
        coin += yellowCount;
        CCString *coinValue = CCString::createWithFormat("%d", coin);
        coinCount->setString(coinValue->getCString());
    }
}

// ブロック削除後のアニメーション
void GameScene::movingBlocksAnimation(list<int> blocks)
{
    // 削除された場所に既存のブロックをずらす
    searchNewPosition(blocks);

    // 新しい位置がセットされたブロックのアニメーション
    moveBlock();
    
    // 新しいブロックを場外に追加
    dropNewBlocks();
    
    // 場外から落とす
    moveBlock();
    
#pragma mark checkDuring
    scheduleOnce(schedule_selector(GameScene::dropAnimationFinished), MOVING_TIME * 2);
}

// 消えたブロックを埋めるように新しい位置をセット
void GameScene::searchNewPosition(list<int> blocks)
{
    // 消えるブロック数分のループ
    list<int>::iterator it1 = blocks.begin();
    while (it1 != blocks.end())
    {
        PositionIndex posIndex1 = getPositionIndex(*it1);
        
        //消えるブロックより上にあるブロックを下にずらす(ポジションのセット)
        for (int x = 0; x < MAX_BLOCK_X; x++) {
            for (int y = 0; y < MAX_BLOCK_Y; y++) {
                if (posIndex1.x == x && posIndex1.y < y) {
                    int tag = getTag(x, y);
                    BlockSprite *blockSprite = (BlockSprite*)m_background->getChildByTag(tag);
                    if(blockSprite != NULL) {
                        PositionIndex pos = getPositionIndex(tag);
                        setNewPosition(tag, pos);
                    }
                }
            }
        }
        it1++;
    }
}

// 新しい位置をセット
void GameScene::setNewPosition(int tag, PositionIndex posIndex)
{
    BlockSprite* blockSprite = (BlockSprite*)m_background->getChildByTag(tag);
    
    
    int nextPosY = blockSprite->getNextPosY();
    if (nextPosY == -1)
    {
        nextPosY = posIndex.y;
    }
    
    // 移動先の位置をセット
    blockSprite->setNextPos(posIndex.x, --nextPosY);
}


// ブロックを移動する
void GameScene::moveBlock()
{
    for(int x = 0; x < MAX_BLOCK_X; x++) {
        for( int y = 0; y < MAX_BLOCK_Y; y++) {
            int tag = getTag(x, y);
            BlockSprite *blockSprite = (BlockSprite *)m_background->getChildByTag(tag);
            if(blockSprite != NULL) {
                int nextPosX = blockSprite->getNextPosX();
                int nextPosY = blockSprite->getNextPosY();
                
                if(nextPosX != -1 || nextPosY != -1) {
                    CCPoint nowPosition = getPosition(blockSprite->getIndexX(), blockSprite->getIndexY());
                    CCPoint nextPosition = getPosition(nextPosX, nextPosY);

                    int newTag = getTag(nextPosX, nextPosY);
                    blockSprite->initNextPos();
                    blockSprite->setTag(newTag);
                    blockSprite->setIndexX(nextPosX);
                    blockSprite->setIndexY(nextPosY);
                    //移動中は触らせない
                    blockSprite->setIsTouchFlag(false);
                    CCMoveBy* move = CCMoveBy::create(MOVING_TIME, ccp(nextPosition.x - nowPosition.x, nextPosition.y - nowPosition.y));
                    blockSprite->runAction(move);
                }
            }
        }
    }
    
}

//新しいブロックを落とす
void GameScene::dropNewBlocks()
{
    for (int x = 0; x < MAX_BLOCK_X; x++) {
        int removedCount = 0;
        for (int y = 0; y < MAX_BLOCK_Y; y++) {
            int tag = getTag(x, y);
            BlockSprite *bSprite = (BlockSprite *)m_background->getChildByTag(tag);
            //消えたスプライトの数を取得
            if (bSprite == NULL) {
                removedCount++;
            }
        }
        
        //追加
        for (int i = 0; 0 < removedCount; removedCount--, i++) {
            kBlock blockType = (kBlock)(rand() % kBlockCount);
            
            //列の空きの中でも上から順番にタグをセットする.
            int tag = getTag(x, MAX_BLOCK_Y - removedCount);
            
            BlockSprite *pBlock = BlockSprite::createWithBlockType(blockType);
            //画面外に配置
            pBlock->setPosition(getPosition(x, MAX_BLOCK_Y + i));
            //落ちる目的地はタグの場所
            pBlock->setNextPos(x, MAX_BLOCK_Y - removedCount);
            pBlock->setIndexX(x);
            pBlock->setIndexY(MAX_BLOCK_Y + i);
            pBlock->setIsTouchFlag(false);
            m_background->addChild(pBlock, kZOrderBlock, tag);
        }
        
    }
    
}

// ブロックの移動完了
void GameScene::dropAnimationFinished()
{
    // タッチできないブロックをタッチできるように設定
    for (int x = 0; x < MAX_BLOCK_X; x++) {
        for (int y = 0; y < MAX_BLOCK_Y; y++) {
            int tag = getTag(x, y);
            BlockSprite *bSprite = (BlockSprite *)m_background->getChildByTag(tag);
            if (!(bSprite->getIsTouchFlag())) {
                bSprite->setIsTouchFlag(true);
            }
        }
    }

    // 全探索を行う
    isChainFlag = true;
    
    // 続けて連結があるかチェックして、消す
    // 消せなければアニメーション終了
    checkAndRemoveAndDrop();
}

// ブロックのインデックス取得
GameScene::PositionIndex GameScene::getPositionIndex(int tag)
{
    int pos1_x = (tag - kTagBaseBlock) / 100;
    int pos1_y = (tag - kTagBaseBlock) % 100;
    
    return PositionIndex(pos1_x, pos1_y);
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
        PositionIndex nextToBlockIndex = getPositionIndex(nextToBlockTag);

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
    resetButton->setPosition(ccp(winSize.width - resetButton->getContentSize().width / 2,
                                 resetButton->getContentSize().height / 2));
    
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
