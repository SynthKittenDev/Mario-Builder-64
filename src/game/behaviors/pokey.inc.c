
/**
 * Behavior for bhvPokey and bhvPokeyBodyPart.
 * bhvPokey is responsible for the behavior of the pokey itself, as well as
 * spawning the body parts.
 * Pokey comes before its body parts in processing order, and the body parts
 * are processed top to bottom.
 */

/**
 * Hitbox for a single pokey body part.
 */
static struct ObjectHitbox sPokeyBodyPartHitbox = {
    /* interactType:      */ INTERACT_BOUNCE_TOP,
    /* downOffset:        */ 10,
    /* damageOrCoinValue: */ 2,
    /* health:            */ 0,
    /* numLootCoins:      */ 0,
    /* radius:            */ 40,
    /* height:            */ 30,
    /* hurtboxRadius:     */ 42,
    /* hurtboxHeight:     */ 20,
};

/**
 * Attack handlers for pokey body part.
 */
static u8 sPokeyBodyPartAttackHandlers[] = {
    /* ATTACK_PUNCH:                 */ ATTACK_HANDLER_KNOCKBACK,
    /* ATTACK_KICK_OR_TRIP:          */ ATTACK_HANDLER_KNOCKBACK,
    /* ATTACK_FROM_ABOVE:            */ ATTACK_HANDLER_DIE_IF_HEALTH_NON_POSITIVE,
    /* ATTACK_GROUND_POUND_OR_TWIRL: */ ATTACK_HANDLER_DIE_IF_HEALTH_NON_POSITIVE,
    /* ATTACK_FAST_ATTACK:           */ ATTACK_HANDLER_KNOCKBACK,
    /* ATTACK_FROM_BELOW:            */ ATTACK_HANDLER_KNOCKBACK,
};

/**
 * Update function for pokey body part.
 * The behavior parameter is the body part's index from POKEY_PART_BP_HEAD to POKEY_PART_BP_LOWEST,
 * with POKEY_PART_BP_HEAD at the top.
 */
void bhv_pokey_body_part_update(void) {
    // PARTIAL_UPDATE
    o->oQuicksandDepthToDie = 0;
    if (obj_update_standard_actions(3.0f)) {
        if (o->parentObj->oAction == POKEY_ACT_UNLOAD_PARTS) {
            obj_mark_for_deletion(o);
        } else {
            cur_obj_update_floor();
            if (SURFACE_IS_BURNING(o->oFloorType) && (o->oPosY < o->oFloorHeight + 5.f)) {
                o->oMoveFlags = OBJ_MOVE_ABOVE_LAVA | OBJ_MOVE_ON_GROUND;
            }

            obj_update_blinking(&o->oPokeyBodyPartBlinkTimer, 30, 60, 4);

            // If the body part above us is dead, then decrease body part index
            // by one, since new parts are spawned from the bottom.
            //! It is possible to briefly get two body parts to have the same
            //  index by killing two body parts on the frame before a new part
            //  spawns, but one of the body parts shifts upward immediately,
            //  so not very interesting
            if (o->oBehParams2ndByte > 1
                && !(o->parentObj->oPokeyAliveBodyPartFlags & (1 << (o->oBehParams2ndByte - 1)))) {
                o->parentObj->oPokeyAliveBodyPartFlags =
                    o->parentObj->oPokeyAliveBodyPartFlags | 1 << (o->oBehParams2ndByte - 1);

                o->parentObj->oPokeyAliveBodyPartFlags =
                    o->parentObj->oPokeyAliveBodyPartFlags & ((1 << o->oBehParams2ndByte) ^ ~0);

                o->oBehParams2ndByte--;
            }

            // Set the bottom body part size, and gradually increase it.
            //! This "else if" means that if a body part above the expanding
            //  one dies, then the expanding will pause for one frame.
            //! If you kill a body part as it's expanding, the body part that
            //  was above it will instantly shrink and begin expanding in its
            //  place.
            else if (o->parentObj->oPokeyBottomBodyPartSize < 1.0f
                     && o->oBehParams2ndByte + 1 == o->parentObj->oPokeyNumAliveBodyParts) {
                approach_f32_ptr(&o->parentObj->oPokeyBottomBodyPartSize, 1.0f, 0.1f);
                cur_obj_scale(o->parentObj->oPokeyBottomBodyPartSize * 3.0f);
            }

            s16 offsetAngle = o->oBehParams2ndByte * 0x4000 + o->oTimer * 0x800;
            o->oPosX = o->parentObj->oPosX + coss(offsetAngle) * 6.0f;
            o->oPosZ = o->parentObj->oPosZ + sins(offsetAngle) * 6.0f;

            // This is the height of the tower beneath the body part
            f32 baseHeight = o->parentObj->oPosY
                         + (120 * (o->parentObj->oPokeyNumAliveBodyParts - o->oBehParams2ndByte) - 240)
                         + 120.0f * o->parentObj->oPokeyBottomBodyPartSize;
            baseHeight -= o->parentObj->oQuicksandDepth;

            // We treat the base height as a minimum height, allowing the body
            // part to briefly stay in the air after a part below it dies
            if (o->oPosY < baseHeight) {
                o->oPosY = baseHeight;
                o->oVelY = 0.0f;
            }

            // Only the head has loot coins
            if ((o->oBehParams2ndByte == POKEY_PART_BP_HEAD) && (o->parentObj->oImbue == IMBUE_NONE)) {
                o->oNumLootCoins = -1;
            } else {
                o->oNumLootCoins = 0;
            }

            // If the body part was attacked, then die. If the head was killed,
            // then die after a delay.

            if ((o->oBehParams2ndByte == o->parentObj->oPokeyNumAliveBodyParts-1)&&(o->parentObj->oQuicksandDepth > 120)) {
                o->parentObj->oQuicksandDepth = 0;
                o->parentObj->oPokeyNumAliveBodyParts--;
                o->parentObj->oPokeyAliveBodyPartFlags = o->parentObj->oPokeyAliveBodyPartFlags & ((1 << o->oBehParams2ndByte) ^ ~0);
                if ((o->oBehParams2ndByte == POKEY_PART_BP_HEAD) && (o->parentObj->oImbue == IMBUE_NONE)) {
                    struct Object * coin = spawn_object(o, MODEL_BLUE_COIN, bhvBlueCoinMotos);
                    cur_obj_play_sound_2(SOUND_GENERAL_COIN_SPURT);
                    coin->oForwardVel = 10.0f;
                    coin->oVelY = 30.0f;
                    coin->oMoveAngleYaw = (f32)(o->oFaceAngleYaw + 0x8000) + random_float() * 1024.0f;
                    coin->oPosY += 150.0f;
                }
                mark_obj_for_deletion(o);
            }

            if (obj_handle_attacks(&sPokeyBodyPartHitbox, o->oAction, sPokeyBodyPartAttackHandlers)) {
                o->parentObj->oPokeyNumAliveBodyParts--;
                if (o->oBehParams2ndByte == POKEY_PART_BP_HEAD) {
                    o->parentObj->oPokeyHeadWasKilled = TRUE;
                }

                o->parentObj->oPokeyAliveBodyPartFlags =
                    o->parentObj->oPokeyAliveBodyPartFlags & ((1 << o->oBehParams2ndByte) ^ ~0);
            } else if (o->parentObj->oPokeyHeadWasKilled) {
                cur_obj_become_intangible();

                if (--o->oPokeyBodyPartDeathDelayAfterHeadKilled < 0) {
                    o->parentObj->oPokeyNumAliveBodyParts--;
                    obj_die_if_health_non_positive();
                }
            } else {
                // Die in order from top to bottom
                // If a new body part spawns after the head has been killed, its
                // death delay will be 0
                o->oPokeyBodyPartDeathDelayAfterHeadKilled = (o->oBehParams2ndByte << 2) + 20;
            }
            o->oVelY -= 4.f;
            o->oPosY += o->oVelY;
        }
    } else {
        o->oAnimState = 1;
    }

    o->oGraphYOffset = o->header.gfx.scale[1] * 22.0f;
}

/**
 * When mario gets within range, spawn the POKEY_NUM_SEGMENTS body parts and enter the wander action.
 */
static void pokey_act_uninitialized(void) {
    struct Object *bodyPart;
    o->oQuicksandDepthToDie = 254;
    o->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_NONE];

    if (o->oDistanceToMario < o->oDrawingDistance) {
        ModelID16 partModel = MODEL_MAKER_POKEY_HEAD;
        s32 i;

        for (i = 0; i < POKEY_NUM_SEGMENTS; i++) {
            // Spawn body parts at y offsets 480, 360, 240, 120, 0
            // behavior param POKEY_PART_BP_HEAD = head, POKEY_PART_BP_LOWEST = lowest body part
            bodyPart = spawn_object_relative(i, 0, -i * 120 + 480, 0, o, partModel, bhvPokeyBodyPart);

            if (bodyPart != NULL) {
                obj_scale(bodyPart, 3.0f);
            }

            partModel = MODEL_MAKER_POKEY_BODY;
        }

        o->oPokeyAliveBodyPartFlags = BITMASK(POKEY_NUM_SEGMENTS);
        o->oPokeyNumAliveBodyParts = POKEY_NUM_SEGMENTS;
        o->oPokeyBottomBodyPartSize = 1.0f;
        o->oAction = POKEY_ACT_WANDER;
    }
}

/**
 * Wander around, replenishing body parts if they are killed. When mario moves
 * far away, enter the unload parts action.
 * While wandering, if mario is within 2000 units, try to move toward him. But
 * if mario gets too close, then shy away from him.
 */
static void pokey_act_wander(void) {
    s32 targetAngleOffset;

    if (o->oPokeyNumAliveBodyParts == POKEY_PART_BP_HEAD) {
        obj_mark_for_deletion(o);
        cur_obj_drop_imbued_object(MB64_STAR_HEIGHT);
    } else if (o->oDistanceToMario > o->oDrawingDistance + 500.0f) {
        o->oAction = POKEY_ACT_UNLOAD_PARTS;
        o->oForwardVel = 0.0f;
    } else {
        cur_obj_update_floor_and_walls();
        cur_obj_set_home_if_safe();

        if (o->oPokeyHeadWasKilled) {
            o->oForwardVel = 0.0f;
        } else {
            o->oForwardVel = 5.0f;

            // If a body part is missing, replenish it after 100 frames
            if (o->oPokeyNumAliveBodyParts < POKEY_NUM_SEGMENTS) {
                if ((o->oTimer > 100) && (o->oFloor->type != SURFACE_INSTANT_QUICKSAND)) {
                    // Because the body parts shift index whenever a body part
                    // is killed, the new part's index is equal to the number
                    // of living body parts

                    struct Object *bodyPart
                        = spawn_object_relative(o->oPokeyNumAliveBodyParts, 0, 0, 0, o,
                                                MODEL_MAKER_POKEY_BODY, bhvPokeyBodyPart);

                    if (bodyPart != NULL) {
                        o->oPokeyAliveBodyPartFlags =
                            o->oPokeyAliveBodyPartFlags | (1 << o->oPokeyNumAliveBodyParts);
                        o->oPokeyNumAliveBodyParts++;
                        o->oPokeyBottomBodyPartSize = 0.0f;

                        obj_scale(bodyPart, 0.0f);
                    }

                    o->oTimer = 0;
                }
            } else {
                o->oTimer = 0;
            }

            if (o->oPokeyTurningAwayFromWall) {
                o->oPokeyTurningAwayFromWall =
                    obj_resolve_collisions_and_turn(o->oPokeyTargetYaw, 0x200);
            } else {
                // If far from home, turn back toward home
                if (o->oDistanceToMario >= 25000.0f) {
                    o->oPokeyTargetYaw = o->oAngleToMario;
                }

                if (!(o->oPokeyTurningAwayFromWall =
                          obj_bounce_off_walls_edges_objects(&o->oPokeyTargetYaw))) {
                    if (o->oPokeyChangeTargetTimer != 0) {
                        o->oPokeyChangeTargetTimer--;
                    } else if (o->oDistanceToMario > 2000.0f) {
                        o->oPokeyTargetYaw = obj_random_fixed_turn(0x2000);
                        o->oPokeyChangeTargetTimer = random_linear_offset(30, 50);
                    } else {
                        // The goal of this computation is to make pokey approach
                        // mario directly if he is far away, but to shy away from
                        // him when he is nearby

                        // targetAngleOffset is 0 when distance to mario is >= 1838.4
                        // and 0x4000 when distance to mario is <= 200
                        targetAngleOffset = (s32)(0x4000 - (o->oDistanceToMario - 200.0f) * 10.0f);
                        if (targetAngleOffset < 0) {
                            targetAngleOffset = 0;
                        } else if (targetAngleOffset > 0x4000) {
                            targetAngleOffset = 0x4000;
                        }

                        // If we need to rotate CCW to get to mario, then negate
                        // the target angle offset
                        if ((s16)(o->oAngleToMario - o->oMoveAngleYaw) > 0) {
                            targetAngleOffset = -targetAngleOffset;
                        }

                        // When mario is far, targetAngleOffset is 0, so he moves
                        // toward him directly. When mario is close,
                        // targetAngleOffset is 0x4000, so he turns 90 degrees
                        // away from mario
                        o->oPokeyTargetYaw = o->oAngleToMario + targetAngleOffset;
                    }
                }

                cur_obj_rotate_yaw_toward(o->oPokeyTargetYaw, 0x200);
            }
        }

        cur_obj_move_standard(-78);

        if (cur_obj_die_if_on_death_barrier(MB64_STAR_HEIGHT)) {
            // Unload everything
            struct ObjectNode *listHead = &gObjectLists[get_object_list_from_behavior(segmented_to_virtual(bhvPokeyBodyPart))];
            struct Object *obj = (struct Object *) listHead->next;

            while (obj != (struct Object *) listHead) {
                if (obj->behavior == segmented_to_virtual(bhvPokeyBodyPart)
                    && obj->parentObj == o) {
                        obj->activeFlags = ACTIVE_FLAG_DEACTIVATED;
                }
                obj = (struct Object *) obj->header.next;
            }
        }
    }
}

/**
 * Move back to home and enter the uninitialized action.
 * The pokey body parts check to see if pokey is in this action, and if so,
 * unload themselves.
 */
static void pokey_act_unload_parts(void) {
    o->oAction = POKEY_ACT_UNINITIALIZED;
}

/**
 * Update function for pokey.
 */
void bhv_pokey_update(void) {
    // PARTIAL_UPDATE

    o->oDeathSound = SOUND_OBJ_POKEY_DEATH;

    switch (o->oAction) {
        case POKEY_ACT_UNINITIALIZED:
            pokey_act_uninitialized();
            break;
        case POKEY_ACT_WANDER:
            pokey_act_wander();
            break;
        case POKEY_ACT_UNLOAD_PARTS:
            pokey_act_unload_parts();
            break;
    }
}
