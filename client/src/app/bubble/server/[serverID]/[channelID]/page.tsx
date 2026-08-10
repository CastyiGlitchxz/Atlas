'use client'
import { getWebSocket } from "../../../../../typescript/websocket";
import React, { use, useState, useEffect, JSX , useRef, memo, useMemo } from "react"
import { eventManager } from "../../../../../typescript/eventsManager";
import { construct_path, globals } from "../../../../../typescript/env";
import styles from "../../../../../stylesheets/css/chat.module.css";
import { get_token, getUserIDCache } from "../../../../../typescript/user";
import { Client, IMessageFormat, Profile } from "../../../../../typescript/interfaces";
import { useRouter } from "next/navigation";
import Image from "next/image";
import { isHyperlink } from "../../../../../typescript/chat";
import { useUserList } from "../userListContext";

function delete_message(messageID: number, em: eventManager) {
    const token = get_token();
    if (!token) return;

    em.emitEvent("delete_message", { token: token, messageID: messageID, Apikey: Client.apikey, cachedUID: getUserIDCache() });
};

const MessageItem = memo(({ 
    content, 
    refMsgData, 
    isCurrentUser, 
    onEdit, 
    onReply, 
    onDelete
}: { 
    content: IMessageFormat, 
    refMsgData?: IMessageFormat, 
    isCurrentUser: boolean,
    onEdit: () => void,
    onReply: () => void,
    onDelete: () => void
}) => {    
    return (
        <div className={styles.message}>
            {refMsgData && (
                <div className={styles.messageReference}>
                    <div style={{
                        width: '20px',
                        height: '15px',
                        borderLeft: '2px solid gray',
                        borderTop: '2px solid gray',
                        borderTopLeftRadius: '10px',
                        marginRight: '8px',
                        marginTop: '10px'
                    }} />
                    <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${refMsgData.picture}`} alt="" width={20} height={20} unoptimized />
                    <p>{refMsgData.displayName} | {refMsgData.content}</p>
                </div>
            )}

            <div className={styles.messageContent}>
                <div className={styles.userMessage}>
                    <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${content.picture}`} alt="" width={50} height={50} unoptimized />
                    <div>
                        <div style={{ display: "flex", gap: "5px"}}>
                            <p><strong>{content.displayName}</strong></p>
                            <p>{content.timestamp}</p>
                        </div>
                        {content.link === null ? 
                            <p style={{whiteSpace: "pre-line"}}>{content["content"]}</p>
                            :
                            <a href={content.link} data-unsafe data-external onClick={(e) => {
                                e.preventDefault();

                                if (e.currentTarget.dataset.unsafe) {
                                    const input = prompt("Unsafe link, user interaction required\nType yes to continue");

                                    if (input === "yes") {
                                        window.open(content.link);
                                    } else {
                                        return;
                                    }
                                }

                                window.open(content.link);
                            }} style={{ color: "white", textDecorationLine: 'underline' }}>{content.content}</a>
                        }
                    </div>
                </div>

                <div className={styles.messageStateItems}>
                    {isCurrentUser &&
                        <button onClick={onEdit}>
                            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-pencil-square" viewBox="0 0 16 16">
                                <path d="M15.502 1.94a.5.5 0 0 1 0 .706L14.459 3.69l-2-2L13.502.646a.5.5 0 0 1 .707 0l1.293 1.293zm-1.75 2.456-2-2L4.939 9.21a.5.5 0 0 0-.121.196l-.805 2.414a.25.25 0 0 0 .316.316l2.414-.805a.5.5 0 0 0 .196-.12l6.813-6.814z"/>
                                <path fillRule="evenodd" d="M1 13.5A1.5 1.5 0 0 0 2.5 15h11a1.5 1.5 0 0 0 1.5-1.5v-6a.5.5 0 0 0-1 0v6a.5.5 0 0 1-.5.5h-11a.5.5 0 0 1-.5-.5v-11a.5.5 0 0 1 .5-.5H9a.5.5 0 0 0 0-1H2.5A1.5 1.5 0 0 0 1 2.5z"/>
                            </svg>
                        </button>
                    }
                    <button onClick={onReply}>
                        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-reply" viewBox="0 0 16 16">
                            <path d="M6.598 5.013a.144.144 0 0 1 .202.134V6.3a.5.5 0 0 0 .5.5c.667 0 2.013.005 3.3.822.984.624 1.99 1.76 2.595 3.876-1.02-.983-2.185-1.516-3.205-1.799a8.7 8.7 0 0 0-1.921-.306 7 7 0 0 0-.798.008h-.013l-.005.001h-.001L7.3 9.9l-.05-.498a.5.5 0 0 0-.45.498v1.153c0 .108-.11.176-.202.134L2.614 8.254l-.042-.028a.147.147 0 0 1 0-.252l.042-.028zM7.8 10.386q.103 0 .223.006c.434.02 1.034.086 1.7.271 1.326.368 2.896 1.202 3.94 3.08a.5.5 0 0 0 .933-.305c-.464-3.71-1.886-5.662-3.46-6.66-1.245-.79-2.527-.942-3.336-.971v-.66a1.144 1.144 0 0 0-1.767-.96l-3.994 2.94a1.147 1.147 0 0 0 0 1.946l3.994 2.94a1.144 1.144 0 0 0 1.767-.96z"/>
                        </svg>
                    </button>
                    {isCurrentUser &&
                        <button onClick={onDelete}>
                            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-trash" viewBox="0 0 16 16">
                                <path d="M5.5 5.5A.5.5 0 0 1 6 6v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5m2.5 0a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5m3 .5a.5.5 0 0 0-1 0v6a.5.5 0 0 0 1 0z"/>
                                <path d="M14.5 3a1 1 0 0 1-1 1H13v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V4h-.5a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1H6a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1h3.5a1 1 0 0 1 1 1zM4.118 4 4 4.059V13a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V4.059L11.882 4zM2.5 3h11V2h-11z"/>
                            </svg>
                        </button>
                    }
                </div>
            </div>
        </div>
    );
});

MessageItem.displayName = "MessageItem";
export default function Channel({ params }: { params: Promise<{ channelID: string, serverID: string }> }) {
    const router = useRouter();
    const { channelID, serverID } = use(params);;

    const emRef = useRef<eventManager | null>(null);
    const tokenRef = useRef<string | null>(null);

    if (!emRef.current) emRef.current = new eventManager();
    if (!tokenRef.current) tokenRef.current = get_token();

    const em = emRef.current;
    const token = tokenRef.current;

    const chatRef = useRef<HTMLDivElement>(null);
    const messageBoxRef = useRef<HTMLTextAreaElement>(null);
    const [message, setMessage] = useState<string>("");
    const [chatContent, setChatContent] = useState<IMessageFormat[]>([]);
    const [editorState, setEditorState] = useState({
        mode: "message",
        messageIndicator: "",
        messageID: 0,
    });
    const userList = useUserList();
    const [pageIndex, setPageIndex] = useState<number | null>(null);
    const [usersFilter, setUsersFilter] = useState<Profile[] | null>(null);

    useEffect(() => {
        (async () => {
            const res = await fetch(construct_path("api/messages"), {
                method: "GET",
                headers: {
                    "Content-Type": "application/json",
                    "Channel-ID": channelID,
                    "Page-Index": null,
                    "Authorization": `Bearer ${token}`,
                    "Apikey": Client.apikey,
                },
            });
            const data = await res.json();
            const messages: IMessageFormat[] = data.messages;
            console.log(data)

            setChatContent(messages);
        })();
    }, [channelID]);

    useEffect(() => {
        (async () => {
            em.emitEvent("get_user", { token: token });
        })();
    }, [token]);

    function sendMessage(): void {
        if (message === "" || !message || message.match(/^ +/m)) return;

        const ws = getWebSocket();
        if (!ws || ws.readyState !== WebSocket.OPEN) {
            alert("WebSocket not ready");
            return;
        };
        
        switch (editorState.mode) {
            case "message":
                em.emitEvent("send_message", {
                    token: token,
                    channelID: channelID,
                    serverID: serverID,
                    message: message,
                    link: isHyperlink(message) ? message : null
                });
                break;
                
            case "edit":
                edit_message(editorState.messageID, message);
                break;
                
            case "reply":
                reply_to_message(editorState.messageID, message);
                break;
                    
            default:                  
                em.emitEvent("send_message", {token: token, channelID: channelID, message: message});
                break;
        }

        setMessage("");
    }

    useEffect(() => {
        const ws = getWebSocket();

        const handler = (msg: MessageEvent) => {
            const {event, data} = JSON.parse(msg.data);
            const message: IMessageFormat = data;

            switch(event) {
                case "message":
                    setChatContent(prev => [
                        ...prev,
                        {
                            id: message.id,
                            picture: message.picture,
                            displayName: message.displayName,
                            content: message.content,
                            channelID: channelID,
                            groupID: "",
                            receiverID: "",
                            timestamp: message.timestamp,
                            messageRef: message.messageRef,
                            link: message.link,
                            userID: message.userID
                        }
                    ]);
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

                default:
                    break;
            };
        };

        ws.addEventListener("message", handler);
        
        return () => {
            ws.removeEventListener("message", handler);
        };
    }, [channelID, router]);

    function edit_message(messageID: number, content: (string | JSX.Element)) {
        if (!token) return;   
        if (typeof content === "string") setMessage(content);
        
        em.emitEvent("edit_message", { token: token, messageID: messageID, content: content });
        
        setEditorState(prev => ({
            ...prev,
            mode: "message"
        }));
    };

    function reply_to_message(messageID: number, content: string) {
        if (!token) return;
        
        em.emitEvent("reply_to_message", { token: token, refID: messageID, content: content, channelID: channelID});

        setEditorState(prev => ({
            ...prev,
            mode: "message"
        }));
    };

    useEffect(() => {
        if (chatRef.current) {
            chatRef.current.scrollTop = chatRef.current.scrollHeight;
        }

        if (chatContent && chatContent.length !== 0) {
            setPageIndex(chatContent[0].id);
        }
    }, [chatContent]);

    useEffect(() => {
        const match = message.match(/^@([a-zA-Z0-9]+)/)
        if (match) {
            const all_users: Profile[] = [];
            userList.offline.map(user => all_users.push(user));
            userList.online.map(user => all_users.push(user));
            const usrs = all_users.filter(users => users.displayName.includes(match[1]));
            setUsersFilter(usrs);
        } else {
            setUsersFilter(null);
        }
    }, [message]);

    useEffect(() => {
        const container = chatRef.current;
        if (!container) return;
        
        let isFetching = false;
        
        const handleScroll = async () => {
            if (container.scrollTop <= 25 && !isFetching) {
                isFetching = true;
                
                const previousHeight = container.scrollHeight;
                
                try {
                    const res = await fetch(construct_path("api/messages"), {
                        method: "POST",
                        headers: {
                            "Content-Type": "application/json",
                            "Channel-ID": channelID,
                            "Page-Index": pageIndex.toString(),
                            "Authorization": `Bearer ${token}`,
                            "Apikey": Client.apikey
                        },
                    });
                    const data = await res.json();
                    const newMessages: IMessageFormat[] = data.messages;

                    if (newMessages && newMessages.length > 0) {
                        setChatContent(prev => [...newMessages, ...prev]);
                        setPageIndex(prev => prev - 1);

                        requestAnimationFrame(() => {
                            requestAnimationFrame(() => {
                                container.scrollTop = container.scrollHeight - previousHeight;
                                isFetching = false;
                            });
                        });
                    } else {
                        isFetching = false;
                    }
                } catch (err) {
                    console.error("Failed to load previous messages:", err);
                    isFetching = false;
                }
            }
        };

        container.addEventListener("scroll", handleScroll);
        return () => container.removeEventListener("scroll", handleScroll);
    }, [channelID, pageIndex, token]);

    const messageMap = useMemo(() => {
        const map = new Map<number, IMessageFormat>();
        chatContent.forEach(msg => map.set(msg.id, msg));
        return map;
    }, [chatContent]);

    return (
        <div className={styles.main}>
            <div className={styles.mainChat}>                
                <div className={styles.centerContainer}>
                    <div id={styles.chat} ref={chatRef}>
                        {chatContent && chatContent.map((content) => (
                            <MessageItem 
                                key={content.id}
                                content={content}
                                refMsgData={content.messageRef ? messageMap.get(content.messageRef) : undefined}
                                isCurrentUser={content.userID === getUserIDCache()}
                                onEdit={() => {
                                    setMessage(content.content.toString());

                                    setEditorState({
                                        messageIndicator: "Editing message",
                                        mode: "edit",
                                        messageID: content.id
                                    });
                                }}
                                onReply={() => {
                                    setEditorState({
                                        messageIndicator: "Replying to message",
                                        mode: "reply",
                                        messageID: content.id
                                    });
                                }}
                                onDelete={() => delete_message(content.id, em)}
                            />
                        ))}
                    </div>

                    <div>
                        {usersFilter && usersFilter.map(user => ( 
                            <div key={user.userID}>
                                <p>@{user.displayName}</p>
                            </div>
                        ))}
                    </div>

                    <div className={styles.messageItems}>
                        {editorState.mode !== "message" &&
                            <div className={styles.messageIndicator}>
                                <p>{editorState.messageIndicator}</p>
                                <button onClick={() => {
                                    setEditorState(prev => ({
                                        ...prev,
                                        mode: "message"
                                    }))
                                    setMessage("");
                                }}>X</button>
                            </div>
                        }

                        <div className={styles["message-bar"]}>
                            <input type="file" id="fileUpload" hidden/>
                            <span className={styles.fileUpload}>
                                <label htmlFor="fileUpload">
                                    +
                                </label>
                            </span>

                            <textarea placeholder="Type your message here" onInput={(e) => setMessage(e.currentTarget.value)} value={message} ref={messageBoxRef} onKeyDown={(e) => {
                                if (e.key === "Enter" && !e.shiftKey) {
                                    e.preventDefault();
                                    sendMessage();
                                }
                            }}></textarea>

                            {message.length > 0 && <button onClick={sendMessage} className={styles["send-button"]}>
                                <svg xmlns="http://www.w3.org/2000/svg" width="23" height="23" fill="currentColor" className="bi bi-send" viewBox="0 0 16 16">
                                    <path d="M15.854.146a.5.5 0 0 1 .11.54l-5.819 14.547a.75.75 0 0 1-1.329.124l-3.178-4.995L.643 7.184a.75.75 0 0 1 .124-1.33L15.314.037a.5.5 0 0 1 .54.11ZM6.636 10.07l2.761 4.338L14.13 2.576zm6.787-8.201L1.591 6.602l4.339 2.76z"/>
                                </svg>
                            </button>}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}