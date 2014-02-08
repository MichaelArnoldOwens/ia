#include "Explosion.h"

#include "Engine.h"

#include "FeatureSmoke.h"
#include "Renderer.h"
#include "Map.h"
#include "Log.h"
#include "MapParsing.h"
#include "SdlWrapper.h"
#include "LineCalc.h"
#include "ActorPlayer.h"

namespace {

void draw(const vector< vector<Pos> >& posLists, bool blockers[MAP_W][MAP_H],
          const bool SHOULD_OVERRIDE_CLR, const SDL_Color& clrOverride,
          Engine& eng) {
  eng.renderer->drawMapAndInterface();

  const SDL_Color& clrInner = SHOULD_OVERRIDE_CLR ? clrOverride : clrYellow;
  const SDL_Color& clrOuter = SHOULD_OVERRIDE_CLR ? clrOverride : clrRedLgt;

  const bool IS_TILES     = eng.config->isTilesMode;
  const int NR_ANIM_STEPS = IS_TILES ? 2 : 1;

  bool isAnyCellSeenByPlayer = false;

  for(int iAnim = 0; iAnim < NR_ANIM_STEPS; iAnim++) {

    const Tile tile = iAnim == 0 ? tile_blast1 : tile_blast2;

    const int NR_OUTER = posLists.size();
    for(int iOuter = 0; iOuter < NR_OUTER; iOuter++) {
      const SDL_Color& clr = iOuter == NR_OUTER - 1 ? clrOuter : clrInner;
      const vector<Pos>& inner = posLists.at(iOuter);
      for(const Pos & pos : inner) {
        if(
          eng.map->cells[pos.x][pos.y].isSeenByPlayer &&
          blockers[pos.x][pos.y] == false) {
          isAnyCellSeenByPlayer = true;
          if(IS_TILES) {
            eng.renderer->drawTile(tile, panel_map, pos, clr, clrBlack);
          } else {
            eng.renderer->drawGlyph('*', panel_map, pos, clr, true, clrBlack);
          }
        }
      }
    }
    if(isAnyCellSeenByPlayer) {
      eng.renderer->updateScreen();
      eng.sdlWrapper->sleep(eng.config->delayExplosion / NR_ANIM_STEPS);
    }
  }
}

void getArea(const Pos& c, const int RADI, Rect& rectRef) {
  rectRef = Rect(Pos(max(c.x - RADI, 1),         max(c.y - RADI, 1)),
                 Pos(min(c.x + RADI, MAP_W - 2), min(c.y + RADI, MAP_H - 2)));
}

void getPositionsReached(const Rect& area, const Pos& origin,
                         bool blockers[MAP_W][MAP_H], Engine& eng,
                         vector< vector<Pos> >& posListRef) {
  vector<Pos> line;
  for(int y = area.x0y0.y; y <= area.x1y1.y; y++) {
    for(int x = area.x0y0.x; x <= area.x1y1.x; x++) {
      const Pos pos(x, y);
      const int DIST = eng.basicUtils->chebyshevDist(pos, origin);
      bool isReached = true;
      if(DIST > 1) {
        eng.lineCalc->calcNewLine(origin, pos, true, 999, false, line);
        for(Pos & posCheckBlock : line) {
          if(blockers[posCheckBlock.x][posCheckBlock.y]) {
            isReached = false;
            break;
          }
        }
      }
      if(isReached) {
        if(int(posListRef.size()) <= DIST) {posListRef.resize(DIST + 1);}
        posListRef.at(DIST).push_back(pos);
      }
    }
  }
}

} //namespace


namespace Explosion {

void runExplosionAt(const Pos& origin, Engine& eng, const int RADI_CHANGE,
                    const SfxId sfx, const bool SHOULD_DO_EXPLOSION_DMG,
                    Prop* const prop, const bool SHOULD_OVERRIDE_CLR,
                    const SDL_Color& clrOverride) {
  Rect area;
  const int RADI = EXPLOSION_STD_RADI + RADI_CHANGE;
  getArea(origin, RADI, area);

  bool blockers[MAP_W][MAP_H];
  MapParse::parse(CellPred::BlocksProjectiles(eng), blockers);

  vector< vector<Pos> > posLists;
  getPositionsReached(area, origin, blockers, eng, posLists);

  Sound snd("I hear an explosion!", sfx, true, origin, NULL,
            SHOULD_DO_EXPLOSION_DMG, true);
  eng.soundEmitter->emitSound(snd);

  draw(posLists, blockers, SHOULD_OVERRIDE_CLR, clrOverride, eng);

  //Do damage, apply effect
  const int DMG_ROLLS = 5;
  const int DMG_SIDES = 6;
  const int DMG_PLUS  = 10;

  Actor* actorArray[MAP_W][MAP_H];
  eng.basicUtils->makeActorArray(actorArray);

  const int NR_OUTER = posLists.size();
  for(int curRadi = 0; curRadi < NR_OUTER; curRadi++) {
    const vector<Pos>& inner = posLists.at(curRadi);

    for(const Pos & pos : inner) {

      Actor* actor = actorArray[pos.x][pos.y];

      if(SHOULD_DO_EXPLOSION_DMG) {
        //Damage environment
        if(curRadi <= 1) {eng.map->switchToDestroyedFeatAt(pos);}
        const int DMG = eng.dice(DMG_ROLLS - curRadi, DMG_SIDES) + DMG_PLUS;

        //Damage actor
        if(actor != NULL) {
          if(actor->deadState == actorDeadState_alive) {
            if(actor == eng.player) {
              eng.log->addMsg("I am hit by an explosion!", clrMsgBad);
            }
            actor->hit(DMG, dmgType_physical, true);
          }
        }
        if(eng.dice.fraction(6, 10)) {
          eng.featureFactory->spawnFeatureAt(
            feature_smoke, pos, new SmokeSpawnData(eng.dice.range(2, 4)));
        }
      }

      //Apply property
      if(prop != NULL && actor != NULL) {
        if(actor->deadState == actorDeadState_alive) {
          PropHandler& propHlr = actor->getPropHandler();
          Prop* propCpy =
            propHlr.makePropFromId(prop->getId(), propTurnsSpecified,
                                   prop->turnsLeft_);
          propHlr.tryApplyProp(propCpy);

        }
      }
    }
  }

  eng.player->updateFov();
  eng.renderer->drawMapAndInterface();

  if(prop != NULL) {delete prop;}
}

void runSmokeExplosionAt(const Pos& origin, Engine& eng) {
  Rect area;
  const int RADI = EXPLOSION_STD_RADI;
  getArea(origin, RADI, area);

  bool blockers[MAP_W][MAP_H];
  MapParse::parse(CellPred::BlocksProjectiles(eng), blockers);

  vector< vector<Pos> > posLists;
  getPositionsReached(area, origin, blockers, eng, posLists);

  //TODO Sound message?
  Sound snd("", endOfSfxId, true, origin, NULL, false, true);
  eng.soundEmitter->emitSound(snd);

  for(const vector<Pos>& inner : posLists) {
    for(const Pos & pos : inner) {
      if(blockers[pos.x][pos.y] == false) {
        eng.featureFactory->spawnFeatureAt(
          feature_smoke, pos, new SmokeSpawnData(eng.dice.range(17, 22)));
      }
    }
  }

  eng.player->updateFov();
  eng.renderer->drawMapAndInterface();
}

} //Explosion

