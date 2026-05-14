'use client'
import React, { useEffect, useRef, useState } from 'react';
import styles from "../../stylesheets/css/bubble.module.css";
import { construct_path, globals } from '../../typescript/env';
import { Client, Friend, Friends } from '../../typescript/interfaces';
import { get_token } from '../../typescript/user';
import Image from 'next/image';
import { UserPanel } from '../components';
import { eventManager } from '../../typescript/eventsManager';
import { getWebSocket } from '../../typescript/websocket';

export default function Bubble() {
    const [relationsList, setRelationsList] = useState<Friends | null>(null);
    const [targetUser, setTargetUser] = useState<Friend | null>(null);
    const [requestsShow, setShowRequests] = useState<boolean>(false);

    const emRef = useRef<eventManager | null>(null);
    if (!emRef.current) emRef.current = new eventManager();
    const em = emRef.current;

    useEffect(() => {
        (async () => {
            const token = get_token();

            const friend_res = await fetch(construct_path("api/friends"), {
                method: "GET",
                headers: {
                    "Content-Type": "application/json",
                    "Authorization": `Bearer ${token}`,
                    "Apikey": Client.apikey
                }
            });
            
            const request_res = await fetch(construct_path("api/friends/requests"), {
                method: "GET",
                headers: {
                    "Content-Type": "application/json",
                    "Authorization": `Bearer ${token}`,
                    "Apikey": Client.apikey
                }
            });

            const friends = await friend_res.json();
            const requests = await request_res.json();
            
            if (friend_res.ok && friends !== null) {
                setRelationsList(prev => ({
                    ...prev,
                    friends: friends["friends"],
                }));
            }

            if (request_res.ok && requests !== null) {
                setRelationsList(prev => ({
                    ...prev,
                    requests: requests["requests"],
                }));
            }
        })();
    }, []);

    useEffect(() => {
        console.log("[List]", relationsList);
    }, [relationsList]);

    function messageUser(userID: string) {
        console.log(userID);
    }

    function addFriend(username: string) {
        console.log(username);
    }
    
    function acceptFriendRequest(requestID: number) {
        console.log(requestID);
        em.emitEvent("accept_friend_request", {requestID: requestID});
    }

    function openRequestsList() {
        if (requestsShow === false) {
            setShowRequests(true);
        } else if (requestsShow === true) {
            setShowRequests(false);
        }
    }

    useEffect(() => {
        const ws = getWebSocket();

        const handler = (msg: MessageEvent) => {
            const {event, data} = JSON.parse(msg.data);

            switch(event) {
                case "request_accepted":
                    console.log("called");
                    break;

                default:
                    break;
            };
        };

        ws.addEventListener("message", handler);
        
        return () => {
            ws.removeEventListener("message", handler);
        };
    }, []);

    return (
        <div className={styles.bubble_root}>
            <div className={styles.bubble_pms_list}>
                <strong>Your Messages</strong>

                <div>

                </div>
            </div>

            <div className={styles.bubble_friends_container}>
                <div className={styles["bubble-topbar"]}>
                    <span>
                        <p>Welcome to your personal bubble!</p>
                        <p>Each user (a client) is a point on an Atlas (a collection of maps) contained within their own bubble (houses, personal spaces, country, idk)</p>
                    </span>

                    <button onClick={() => openRequestsList()}>
                        <svg xmlns="http://www.w3.org/2000/svg" width="25" height="25" fill="currentColor" className="bi bi-people-fill" viewBox="0 0 16 16">
                            <path d="M7 14s-1 0-1-1 1-4 5-4 5 3 5 4-1 1-1 1zm4-6a3 3 0 1 0 0-6 3 3 0 0 0 0 6m-5.784 6A2.24 2.24 0 0 1 5 13c0-1.355.68-2.75 1.936-3.72A6.3 6.3 0 0 0 5 9c-4 0-5 3-5 4s1 1 1 1zM4.5 8a2.5 2.5 0 1 0 0-5 2.5 2.5 0 0 0 0 5"/>
                        </svg>
                    </button>
                </div>

                <div className={styles["friends-list"]}>
                    <strong>Friends</strong>
                    <div className={styles["friend-list-content"]}>
                        {relationsList?.friends ? 
                            relationsList.friends.map((friend) => (
                                <div className={styles["bubble-friend"]} key={friend.id}>
                                    <div className={styles.bubble_friend_userinfo}>
                                        <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${friend.picture}`} alt="" width={20} height={20} unoptimized className={styles.bubble_friend_picture} draggable={false}/>
                                        <div>
                                            <strong>{friend.displayName}</strong>
                                            <p>{friend.customStatus}</p>
                                        </div>
                                    </div>


                                    <div className={styles.bubble_friend_options}>
                                        <button className={styles.bubble_friend_options_buttons}>
                                            <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" fill="currentColor" className="bi bi-telephone" viewBox="0 0 16 16">
                                                <path d="M3.654 1.328a.678.678 0 0 0-1.015-.063L1.605 2.3c-.483.484-.661 1.169-.45 1.77a17.6 17.6 0 0 0 4.168 6.608 17.6 17.6 0 0 0 6.608 4.168c.601.211 1.286.033 1.77-.45l1.034-1.034a.678.678 0 0 0-.063-1.015l-2.307-1.794a.68.68 0 0 0-.58-.122l-2.19.547a1.75 1.75 0 0 1-1.657-.459L5.482 8.062a1.75 1.75 0 0 1-.46-1.657l.548-2.19a.68.68 0 0 0-.122-.58zM1.884.511a1.745 1.745 0 0 1 2.612.163L6.29 2.98c.329.423.445.974.315 1.494l-.547 2.19a.68.68 0 0 0 .178.643l2.457 2.457a.68.68 0 0 0 .644.178l2.189-.547a1.75 1.75 0 0 1 1.494.315l2.306 1.794c.829.645.905 1.87.163 2.611l-1.034 1.034c-.74.74-1.846 1.065-2.877.702a18.6 18.6 0 0 1-7.01-4.42 18.6 18.6 0 0 1-4.42-7.009c-.362-1.03-.037-2.137.703-2.877z"/>
                                            </svg>
                                        </button>
                                        <button className={styles.bubble_friend_options_buttons} onClick={() => messageUser(friend.userID)}>
                                            <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" fill="currentColor" className="bi bi-chat" viewBox="0 0 16 16">
                                                <path d="M2.678 11.894a1 1 0 0 1 .287.801 11 11 0 0 1-.398 2c1.395-.323 2.247-.697 2.634-.893a1 1 0 0 1 .71-.074A8 8 0 0 0 8 14c3.996 0 7-2.807 7-6s-3.004-6-7-6-7 2.808-7 6c0 1.468.617 2.83 1.678 3.894m-.493 3.905a22 22 0 0 1-.713.129c-.2.032-.352-.176-.273-.362a10 10 0 0 0 .244-.637l.003-.01c.248-.72.45-1.548.524-2.319C.743 11.37 0 9.76 0 8c0-3.866 3.582-7 8-7s8 3.134 8 7-3.582 7-8 7a9 9 0 0 1-2.347-.306c-.52.263-1.639.742-3.468 1.105"/>
                                            </svg>
                                        </button>
                                        <button className={styles.bubble_friend_options_buttons} onClick={() => setTargetUser(friend)}>
                                            <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" fill="currentColor" className="bi bi-three-dots-vertical" viewBox="0 0 16 16">
                                                <path d="M9.5 13a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0m0-5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0m0-5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0"/>
                                            </svg>
                                        </button>
                                    </div>
                                </div>
                            ))
                        :
                            <p>You have none, sad for you. Look on the bright side tho, you have a reason to touch grass now.</p>
                        }
                    </div>
                </div>
            </div>

            {requestsShow && !targetUser && <div className={styles.requestsList}>
                <p>Friend Requests</p>

                <div>
                    {relationsList?.requests ? 
                        relationsList.requests.map((request) => (
                            <div className={styles.bubble_request} key={request.id}>
                                <div className={styles.bubble_request_userinfo}>
                                    <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${request.picture}`} alt="" width={20} height={20} unoptimized className={styles.bubble_request_picture} draggable={false}/>
                                    <div>
                                        <strong>{request.displayName}</strong>
                                        <p>{request.customStatus}</p>
                                    </div>
                                </div>


                                <div className={styles.bubble_request_options}>
                                    <button onClick={() => acceptFriendRequest(request.id)}>+</button>
                                    <button>-</button>
                                </div>
                            </div>
                        ))
                    :
                        <p>No friend requests.</p>
                    }
                </div>
            </div>}
            {targetUser !== null && <UserPanel user={targetUser} setTargetUser={setTargetUser}/>}
        </div>
    )
}