'use client'
import { getWebSocket } from "../../../../typescript/websocket";
import React, { use, useState, useEffect, JSX , useRef, useContext} from "react"
import { eventManager } from "../../../../typescript/eventsManager";
import { construct_path, globals } from "../../../../typescript/env";
import styles from "../../../../stylesheets/css/chat.module.css";
import { get_token, open_profile } from "../../../../typescript/user";
import { Profile } from "../../../..//typescript/interfaces";
import { useRouter } from "next/navigation";
import { ProfilePanel } from "../../layout";
import Image from "next/image";
import { messageFormat } from "../../../../typescript/interfaces";
import { isSameChat, isHyperlink } from "../../../../typescript/chat";

function delete_message(message_id: string) {
    const em = new eventManager();

    const token = get_token();
    if (!token) return;

    em.emitEvent("delete_message", { auth: token, message_id: message_id });
};

export default function Chat({ params }: { params: Promise<{ id: string }> }) {
    const router = useRouter();
    const { id } = use(params);
    const emRef = useRef<eventManager | null>(null);
    const tokenRef = useRef<string | null>(null);

    if (!emRef.current) {
        emRef.current = new eventManager();
    }

    if (!tokenRef.current) {
        tokenRef.current = get_token();
    }

    const em = emRef.current;
    const token = tokenRef.current;

    const sid: string = id;

    const chatRef = useRef<HTMLDivElement>(null);
    const canvasRef = useRef<HTMLCanvasElement[]>([]);
    const [message, setMessage] = useState<string>("");
    const [chatContent, setChatContent] = useState<messageFormat[]>([]);
    const [userList, setUserList] = useState<{online: Profile[], offline: Profile[]}>({
        online: [],
        offline: []
    });
    const [messageMode, setMessageMode] = useState<string>("message");
    const [indicatorMessage, setIndicatorMessage] = useState<string>("");
    const [mid, setMid] = useState<number>(0);
    const [user, setUser] = useState<string>("");
    const [pageIndex, setPageIndex] = useState<number>(null);
    const ctx = useContext(ProfilePanel);
    if (!ctx) {
        throw new Error("ProfilePanel must be used within a ProfilePanel.Provider");
    }

    const { setPreview, setShowPreview } = ctx;

    useEffect(() => {
        (async () => {
            const res = await fetch(construct_path("api/messages_get"), {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({ sid: sid, index: pageIndex }),
            });
            const data = await res.json();
            const messages: messageFormat[] = data.messages.messages;

            setChatContent(messages);
        })();
    }, [sid]);
    
    useEffect(() => {
        (async () => {
            em.emitEvent("get_user", { token: token });
        })();
    }, [chatContent, user, sid]);

    function sendMessage(): void {
        if (message === "" || !message) return;

        const ws = getWebSocket();
        if (!ws || ws.readyState !== WebSocket.OPEN) {
            alert("WebSocket not ready");
            return;
        };
        
        switch (messageMode) {
            case "message":
                triggerNotification(message);
                em.emitEvent("send_message", {
                    token: token,
                    sid: sid,
                    message: message,
                    link: isHyperlink(message) ? message : null
                });
                break;
                
            case "edit":
                edit_message(mid, message);
                break;
                
            case "reply":
                reply_to_message(mid, message);
                break;
                    
            default:                  
                triggerNotification(message);
                em.emitEvent("send_message", {token: token, sid: sid, message: message});
                break;
        }

        setMessage("");
    }

    function triggerNotification(content: string) {
        if (!("Notification" in window)) {
            return;
        };

        em.emitEvent("schedule_notification", { token: token, content: content, sid: sid });
    };
    
    useEffect(() => {
        const ws = getWebSocket();
    
        function update_userlist(userID: string, status: "online" | "offline" | "idle") {
            setUserList(prev => {
                const userOnline = prev.online.find(u => u.userid === userID);
                const userOffline = prev.offline.find(u => u.userid === userID);

                if (status === "offline") {
                    if (userOnline) {
                        return {
                            online: prev.online.filter(u => u.userid !== userID),
                            offline: [...prev.offline, {...userOnline, status: "offline"}],
                        };
                    }
                    return prev;
                }

                const existingUser = userOnline || userOffline;
                if (!existingUser) return prev; // User not found, do nothing

                const updatedUser = { ...existingUser, status };

                return {
                    online: userOnline
                        ? prev.online.map(u => (u.userid === userID ? updatedUser : u))
                        : [...prev.online, updatedUser],
                    offline: prev.offline.filter(u => u.userid !== userID),
                };
            });
        }

        function addMessageToChat(message: messageFormat) {
            setChatContent(prev => [
                ...prev,
                {
                    id: message.id,
                    picture: message.picture,
                    displayName: message.displayName,
                    content: message.content,
                    serverID: sid,
                    timestamp: message.timestamp,
                    messageRef: message.messageRef,
                    link: message.link
                }
            ]);
        };

        const handler = (msg: MessageEvent) => {
            const {event, data} = JSON.parse(msg.data);

            const message: messageFormat = data;

            switch(event) {
                case "message":
                    
                    if (isSameChat(message.serverID, sid) === false) return;
                    addMessageToChat(message);
                    break;

                case "message_deleted":
                    setChatContent(prev => prev.filter(msg => msg.id !== data.id));
                    break;

                case "message_edited":
                    setChatContent(prev =>
                        prev.map(msg =>
                            msg.id === data.id
                                ? { ...msg, content: data.content }
                                : msg
                        )
                    );
                    break;

                case "update":
                    update_userlist(data.update.userID, data.update.status);
                    break;

                case "return_user":
                    setUser(data.displayName);
                    break;

                default:
                    break;
            };
        };

        ws.addEventListener("message", handler);
        
        return () => {
            ws.removeEventListener("message", handler);
        };
    }, [sid, userList, router]);

    function edit_message(message_id: number, content: (string | JSX.Element)) {
        if (!token) return;   
        if (typeof content === "string") setMessage(content);
        
        em.emitEvent("edit_message", { auth: token, message_id: message_id.toString(), content: content });
        
        setMessageMode("message");
    };

    function reply_to_message(message_id: number, content: string) {
        if (!token) return;
        
        em.emitEvent("reply_to_message", { token: token, ref_id: message_id.toString(), content: content, sid: sid});

        setMessageMode("message");
    };

    useEffect(() => {       
        (async () => {
            const res = await fetch(construct_path("api/servers/userlist_get"), {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({ "serverID": sid }),
            });
            
            const data = await res.json();
            const online_users = data.users.user_list.filter((user: { status: string }) => user.status == "online" || user.status == "idle");
            const offline_users = data.users.user_list.filter((user: { status: string; }) => user.status == "offline");
            setUserList({
                online: online_users,
                offline: offline_users
            });
        })();
    }, [sid]);

    useEffect(() => {
        if (chatRef.current) {
            // Scroll to the bottom whenever messages change
            chatRef.current.scrollTop = chatRef.current.scrollHeight;
        }
    }, [chatContent]);

    useEffect(() => {
        if (chatContent && chatContent.length !== 0) {
            setPageIndex(chatContent[0].id);
        }
    }, [chatContent]);

    useEffect(() => {
        const container = chatRef.current;
        if (!container) return;
        
        let loading = false;
        
        const handleScroll = async () => {
            // Check if we are near the top
            if (container.scrollTop <= 25 && !loading) {
                loading = true;
                
                // Keep track of scroll height before fetching
                const previousHeight = container.scrollHeight;
                
                try {
                    const res = await fetch(construct_path("api/messages_get"), {
                        method: "POST",
                        headers: {
                            "Content-Type": "application/json"
                        },
                        body: JSON.stringify({ sid: sid, index: pageIndex - 1 }) // load previous page
                    });
                    const data = await res.json();
                    const newMessages: messageFormat[] = data.messages.messages;

                    if (newMessages && newMessages.length > 0) {
                        setChatContent(prev => [...newMessages, ...prev]);
                        setPageIndex(prev => prev - 1);

                        container.scrollTop = container.scrollHeight - previousHeight;
                    }
                } catch (err) {
                    console.error("Failed to load previous messages:", err);
                }

                loading = false;
            }
        };

        container.addEventListener("scroll", handleScroll);
        return () => container.removeEventListener("scroll", handleScroll);
    }, [sid, pageIndex]);

    useEffect(() => {
        if (!canvasRef.current) return;

        canvasRef.current.forEach((canvas) => {
            if (!canvas) return;
            const ctx = canvas.getContext("2d");
            if (!ctx) return;

            // Reset transforms and clear previous drawing
            ctx.setTransform(1, 0, 0, 1, 0, 0);
            ctx.clearRect(0, 0, canvas.width, canvas.height);

            // Set origin to bottom-left
            ctx.translate(0, canvas.height);
            ctx.scale(1, -1);

            // Draw curved line
            ctx.beginPath();
            ctx.moveTo(20, 0);
            ctx.quadraticCurveTo(20, 14, 70, 10);
            ctx.strokeStyle = "gray";
            ctx.lineWidth = 2;
            ctx.stroke();
        });
    }, [chatContent]);

    return (
        <div className={styles.main}>
            <div className={styles.centerContainer}>
                <div id={styles.chat} ref={chatRef}>
                    {chatContent && chatContent.map((content) => (
                        <div key={content.id} className={styles.message}>
                            {content.messageRef !== null && 
                                <div>
                                    {chatContent.filter(refMsg => refMsg.id == content.messageRef).map((refMsgData, index) => (
                                        <div key={index} className={styles.messageReference}>
                                            <canvas ref={(el) => {if (el) canvasRef.current[refMsgData.id] = el}} width={40} height={20}></canvas>
                                            <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${refMsgData["picture"]}`} alt="" width={20} height={20} unoptimized quality={1}/>
                                            <p>{refMsgData.displayName}</p>
                                            <p> | {refMsgData.content}</p>
                                        </div>
                                    ))}
                                </div>
                            }

                            <div className={styles.messageContent}>
                                <div className={styles.userMessage}>
                                    <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${content["picture"]}`} alt="" width={50} height={50} unoptimized quality={1}/>
                                    <div>
                                        <div style={{ display: "flex", gap: "5px"}}>
                                            <p>{content["displayName"]}</p>
                                            *
                                            <p>{content["timestamp"]}</p>
                                        </div>
                                        {content.link === null ? 
                                            <p style={{whiteSpace: "pre-line"}}>{content["content"]}</p>
                                            :
                                            <a href={content.link} data-external onClick={(e) => {
                                                e.preventDefault();
                                                console.log(message.link)
                                            }} style={{ color: "white", textDecorationLine: 'underline' }}>{content.content}</a>

                                        }
                                    </div>
                                </div>

                                <div className={styles.messageStateItems}>
                                    { content.displayName === user &&
                                        <button onClick={() => {
                                            setMessage(content["content"].toString());
                                            setIndicatorMessage("Editing message");
                                            setMessageMode("edit");
                                            setMid(content["id"]);
                                        }}>
                                            <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" fill="currentColor" className="bi bi-pencil-square" viewBox="0 0 16 16">
                                                <path d="M15.502 1.94a.5.5 0 0 1 0 .706L14.459 3.69l-2-2L13.502.646a.5.5 0 0 1 .707 0l1.293 1.293zm-1.75 2.456-2-2L4.939 9.21a.5.5 0 0 0-.121.196l-.805 2.414a.25.25 0 0 0 .316.316l2.414-.805a.5.5 0 0 0 .196-.12l6.813-6.814z"/>
                                                <path fillRule="evenodd" d="M1 13.5A1.5 1.5 0 0 0 2.5 15h11a1.5 1.5 0 0 0 1.5-1.5v-6a.5.5 0 0 0-1 0v6a.5.5 0 0 1-.5.5h-11a.5.5 0 0 1-.5-.5v-11a.5.5 0 0 1 .5-.5H9a.5.5 0 0 0 0-1H2.5A1.5 1.5 0 0 0 1 2.5z"/>
                                            </svg>
                                        </button>
                                    }

                                    <button onClick={() => {
                                        setMessageMode("reply");
                                        setIndicatorMessage("Replying to message");
                                        setMid(content["id"]);
                                    }}>
                                        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" fill="currentColor" className="bi bi-arrow-90deg-left" viewBox="0 0 16 16">
                                            <path fillRule="evenodd" d="M1.146 4.854a.5.5 0 0 1 0-.708l4-4a.5.5 0 1 1 .708.708L2.707 4H12.5A2.5 2.5 0 0 1 15 6.5v8a.5.5 0 0 1-1 0v-8A1.5 1.5 0 0 0 12.5 5H2.707l3.147 3.146a.5.5 0 1 1-.708.708z"/>
                                        </svg>
                                    </button>

                                    { content.displayName === user &&
                                        <button onClick={() => {
                                            delete_message(content["id"].toString());
                                        }}>
                                            <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" fill="currentColor" className="bi bi-trash" viewBox="0 0 16 16">
                                                <path d="M5.5 5.5A.5.5 0 0 1 6 6v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5m2.5 0a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5m3 .5a.5.5 0 0 0-1 0v6a.5.5 0 0 0 1 0z"/>
                                                <path d="M14.5 3a1 1 0 0 1-1 1H13v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V4h-.5a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1H6a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1h3.5a1 1 0 0 1 1 1zM4.118 4 4 4.059V13a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V4.059L11.882 4zM2.5 3h11V2h-11z"/>
                                            </svg>
                                        </button>
                                    }
                                </div>
                            </div>
                        </div>
                    ))}
                </div>

                <div className={styles.messageItems}>
                    <input type="file" id="fileUpload" hidden/>
                    <span className={styles.fileUpload}>
                        <label htmlFor="fileUpload">
                            +
                        </label>
                    </span>

                    {messageMode !== "message" &&
                        <div className={styles.messageIndicator}>
                            <p>{indicatorMessage}</p>
                            <button onClick={() => {
                                setMessageMode("message");
                                setMessage("");
                            }}>X</button>
                        </div>
                    }

                    <div className={styles.messageBar}>
                        <textarea placeholder="Type your message here" onInput={(e) => setMessage(e.currentTarget.value)} value={message} onKeyDown={(e) => {
                            if (e.key === "Enter" && !e.shiftKey) {
                                e.preventDefault();
                                sendMessage();
                            }
                        }}></textarea>
                        <button onClick={sendMessage} className={styles.sendButton}>
                            <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="currentColor" className="bi bi-send" viewBox="0 0 16 16">
                                <path d="M15.854.146a.5.5 0 0 1 .11.54l-5.819 14.547a.75.75 0 0 1-1.329.124l-3.178-4.995L.643 7.184a.75.75 0 0 1 .124-1.33L15.314.037a.5.5 0 0 1 .54.11ZM6.636 10.07l2.761 4.338L14.13 2.576zm6.787-8.201L1.591 6.602l4.339 2.76z"/>
                            </svg>
                        </button>
                    </div>
                </div>
            </div>

            <div className={styles.userList}>
                <p>Online -- {userList.online.length}</p>
                {userList.online.map((user, index) => (
                    <div key={index} className={styles.userListUser} onClick={() => open_profile(user, setPreview, setShowPreview)}>
                        <div style={{position: "relative"}}>
                            <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${user["picture"]}`} alt="" width={50} height={50} unoptimized quality={1}/>
                            <span className={`${styles.si} ${styles[user["status"]]}`}></span>
                        </div>
                        <div>
                            <p>{user.displayName}</p>
                            <p className={styles.userStatus}>{user.customStatus}</p>
                        </div>
                    </div>
                ))}
                <p>Offline -- {userList.offline.length}</p>
                {userList.offline.map((user, index) => (
                    <div key={index} className={styles.userListUser} onClick={() => open_profile(user, setPreview, setShowPreview)}>
                        <div style={{position: "relative"}}>
                            <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${user["picture"]}`} alt="" width={50} height={50} unoptimized quality={1}/>
                            <span className={`${styles.si} ${styles[user["status"]]}`}></span>
                        </div>
                        <div>
                            <p>{user.displayName}</p>
                            <p className={styles.userStatus}>{user.customStatus}</p>
                        </div>
                    </div>
                ))}
            </div>
        </div>
    )
}