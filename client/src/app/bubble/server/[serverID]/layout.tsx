'use client'
import React, { Dispatch, SetStateAction, use, useEffect, useRef, useState } from "react"
import { Client, IChannel, IServerData, Profile } from "../../../../typescript/interfaces";
import { get_token, getUserIDCache, setUserIDCache } from "../../../../typescript/user";
import { construct_path, globals } from "../../../../typescript/env";
import styles from "../../../../stylesheets/css/chat.module.css";
import { getWebSocket } from "../../../../typescript/websocket";
import { useParams, useRouter } from "next/navigation";
import Image from "next/image";
import { AtlasInput, Tab, Tabs, UserPanel } from "../../../components";
import { eventManager } from "../../../../typescript/eventsManager";
import { UserListProvider } from "./userListContext";

function create_server_invite_code(em: eventManager, sid: string) {
    const token = get_token();
    em.emitEvent('create_server_invite', { sid: sid, token: token });
}

function leaveServer(em: eventManager, token: string, serverID: string) {
    em.emitEvent("leave_server", { Apikey: Client.apikey, token: token, serverID: serverID });
}

function kickUser(em: eventManager, currentUserID: string, targetUserID: string, serverID: string, reason: string) {
    em.emitEvent("kick_from_server", {
        Apikey: Client.apikey,
        currentUserID: get_token(),
        targetUserID: targetUserID,
        serverID: serverID,
        reason: reason,
    });
}

// function banUser(em: eventManager, userID: string, reason: string) {

// }

// function timeoutUser(em: eventManager, userID: string, reason: string) {

// }

function ChannelCreationPanel({ em, serverID, setChannelPanel } : { em: eventManager, serverID: string, setChannelPanel: Dispatch<SetStateAction<boolean>> } ) {
    const [channelDetails, setChannelDetails] = useState<{ channelName: string }>({
        channelName: "New Channel"
    });

    function createServer() {
        em.emitEvent("create_channel", {channelName: channelDetails.channelName, serverID: serverID});
    }

    return (
        <div className={styles["channel-creation-panel"]}>
            <button onClick={() => setChannelPanel(false)}>close</button>
            <AtlasInput title="Channel Name" value={channelDetails.channelName} onChange={(e) => setChannelDetails(prev => ({
                ...prev,
                channelName: e.target.value
            }))}/>
            <button onClick={() => createServer()}>Create</button>
        </div>
    )
}

export default function ServerLayout({ children, params }: { children, params: Promise<{ serverID: string }> }) {
    const router = useRouter();
    const { serverID } = use(params);
    const clientParams = useParams(); 
    const channelID = clientParams?.channelID as string;

    const emRef = useRef<eventManager | null>(null);
    const tokenRef = useRef<string | null>(null);

    if (!emRef.current) emRef.current = new eventManager();
    if (!tokenRef.current) tokenRef.current = get_token();

    const em = emRef.current;
    const token = tokenRef.current;

    const [server, setServer] = useState<IServerData | null>(null);
    const [userList, setUserList] = useState<{online: Profile[], offline: Profile[]}>({
        online: [],
        offline: []
    });
    const [serverCodes, setServerCodes] = useState<{issued_by: { displayName: string, userID: string }, invite_code: string, expires: string}[] | null>(null);
    const [targetUser, setTargetUser] = useState<Profile | null>(null);
    const [selectedTab, setSelectedTab] = useState<number>(0);
    const [channelPanel, setChannelPanel] = useState<boolean>(false);
    const [selectedChannel, setSelectedChannel] = useState<IChannel | null>(null);
    const [serverSettings, setServerSettings] = useState<boolean>(false);
    const [user, setUser] = useState<{author: {"displayName": string, "userID": string}} | undefined>({author: {
        displayName: "",
        userID: ""
    }});

    useEffect(() => {
        (async () => {
            const res = await fetch(construct_path(`api/servers?sid=${serverID}`), {
                method: "GET",
                headers: {
                    "Content-Type": "application/json",
                    "Authorization": `Bearer ${token}`,
                    "Apikey": Client.apikey,
                },
            });
            const data = await res.json();

            if (res.status === 403 || res.status === 401 || res.status === 404) {
                router.replace("/bubble");
                return;
            }
            
            console.log("Server data: ", data)
            const server: IServerData = data.server;

            setServer(server);
        })();

        em.emitEvent("get_invites", { sid: serverID });
    }, [serverID]);

    useEffect(() => {       
        (async () => {
            const res = await fetch(construct_path("api/servers/userlist"), {
                method: "GET",
                headers: {
                    "Content-Type": "application/json",
                    "Server-ID": serverID,
                    "Authorization": `Bearer ${token}`,
                    "Apikey": Client.apikey,
                },
            });
            
            const data = await res.json();

            const online_users = data.users.user_list.filter((user: { status: string }) => user.status == "online" || user.status == "idle");
            const offline_users = data.users.user_list.filter((user: { status: string; }) => user.status == "offline");
            setUserList({
                online: online_users,
                offline: offline_users
            });
        })();
    }, [serverID]);

    function update_userlist(data) {
        const userID = data.update.userID;
        const status = data.update.status;
        const client = data.update.client;

        console.log(client)

        setUserList(prev => {
            const userOnline = prev.online.find(u => u.userID === userID);
            const userOffline = prev.offline.find(u => u.userID === userID);

            if (status === "offline") {
                if (userOnline) {
                    return {
                        online: prev.online.filter(u => u.userID !== userID),
                        offline: [...prev.offline, { ...userOnline, status: "offline" }],
                    };
                }
                return prev;
            }

            const existingUser = userOnline || userOffline;
            if (!existingUser) return prev;

            const updatedUser = { ...existingUser, status, client };

            return {
                online: userOnline
                    ? prev.online.map(u => (u.userID === userID ? updatedUser : u))
                    : [...prev.online, updatedUser],
                offline: prev.offline.filter(u => u.userID !== userID),
            };
        });
    }

    function updateServer() {
        em.emitEvent("update_server", {
            server: server,
            serverID: serverID
        });
    }

    useEffect(() => {
        const ws = getWebSocket();

        const handler = (msg: MessageEvent) => {
            const {event, data} = JSON.parse(msg.data);

            switch(event) {
                case "update":
                    update_userlist(data);
                    break;

                case "return_user":
                    setUserIDCache(data.author.userID);
                    setUser(data);
                    break;

                case "invite_create":
                    console.log(data);
                    setServerCodes(prev => {
                        if (prev) {
                            return [...prev, data]
                        } else {
                            return [{
                                "issued_by": "",
                                "invite_code": data
                            }];
                        }
                    });
                    break;

                case "invite_deleted":
                    setServerCodes(prev => {
                        if (!prev) return [];
                        return prev.filter(c => c.invite_code !== data);
                    });
                    break;

                case "retreived_invites":
                    console.log("Invites", data);
                    setServerCodes(data.codes);
                    break;

                case "user_left":                    
                    if (getUserIDCache() === data.userID) {
                        router.replace("/bubble");
                        return;
                    }

                    setUserList(prev => {
                        const userID = data.userID;
                        const userOnline = prev.online.find(u => u.userID === userID);
                        const userOffline = prev.offline.find(u => u.userID === userID);

                        if (userOnline || userOffline) {
                            return {
                                online: prev.online.filter(u => u.userID !== userID),
                                offline: prev.offline.filter(u => u.userID !== userID),
                            }
                        }
                    });
                    break;
                
                case "channel_created":
                    setServer(prev => ({
                        ...prev,
                        channels: [...prev.channels, {channelID: data.channelID, channelName: data.channelName}]
                    }))
                    console.log(data)
                    break;

                case "channel_deleted":
                    console.log(data);
                    setServer((prev) => {
                        if (!prev) return null;

                        return {
                            ...prev,
                            channels: prev?.channels?.filter(c => c.channelID !== data.channelID)
                        }
                    });
                    break;

                case "channel_updated":
                    setServer((prev) => {
                        if (!prev) return null;

                        return {
                            ...prev,
                            channels: prev?.channels?.map(c => {
                                if (c.channelID === data.channelID) {
                                    return data;
                                }
                                return c;
                            })
                        }
                    });
                    console.log("Update", data);
                    break;

                default:
                    break;
            };
        };

        ws.addEventListener("message", handler);
        
        return () => {
            ws.removeEventListener("message", handler);
        };
    }, [serverID, userList, router, user]);

    return (
        <div className={styles.main}>
            <div className={styles.contextBar}>
                <p className={styles.serverNameText}>{server === undefined || server === null ? "Loading..." : server.serverName}</p>
                {channelID !== undefined && <span style={{ background: "#7c7eff", padding: 4, borderRadius: 9, border: "1.5px solid #5457ff" }}>
                    <p style={{ color: "white", fontSize: 15, fontWeight: 600 }}>
                        {server === undefined || server === null || server.channels &&
                            server.channels
                                .filter(c => String(c.channelID) === channelID)
                                .map(n => n.channelName)
                        }
                    </p>
                </span>}
            </div>

            {serverSettings === true &&
                <div className={styles["server-settings-panel"]}>
                    <div className={styles["serverSettingsFlexbox"]}>
                        <div className={styles["server-settings-tabs"]}>
                            <button onClick={() => setSelectedTab(0)}>Server</button>
                            <button onClick={() => setSelectedTab(1)}>Channels</button>
                            <button onClick={() => setSelectedTab(2)}>Invites</button>
                            <button onClick={() => setSelectedTab(3)}>Users</button>
                        </div>

                        <Tabs selectedTab={selectedTab} className={styles["server-settings-content-container"]}>
                            <Tab className={styles.serverSettingsSettingsContent}>
                                <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${server["icon"]}`} alt="" width={20} height={20} unoptimized className={styles["server-icon"]} draggable={false}/>
                                <AtlasInput title="Server Name" value={server.serverName} onChange={(e) => setServer(prev => ({
                                    ...prev,
                                    serverName: e.target.value
                                }))}/>

                                <div className={styles["server-owner-card"]}>
                                    <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${server["owner"]["avatar"]}`} alt="" width={50} height={50} unoptimized className={styles["server-owner-avatar"]} draggable={false}/>
                                    <p style={{ fontSize: 17 }}><strong>{server.owner.displayName}</strong> (server owner)</p>
                                </div>
                            </Tab>

                            <Tab className={styles.serverSettingsSettingsContent}>
                                <button onClick={() => {
                                    em.emitEvent("create_channel", {channelName: "New Channel", serverID: serverID});
                                }}>New Channel</button>
                                <button>New Category</button>
                                <div style={{ display: "flex", flexDirection: "row", justifyContent: "space-between" }}>
                                    <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
                                        {server?.channels?.map(channel => (
                                            <div key={channel.channelID} style={{ padding: "0px 4px", width: "400px", height: "40px", display: "flex", flexDirection: "row", justifyContent: "space-between", alignItems: "center", gap: 12 }}>
                                                <div style={{ border: "1px solid gray", borderRadius: 18, width: "100%", height: "100%", display: "flex", alignItems: "center", padding: "0px 12px" }} onClick={() => setSelectedChannel(channel)}>
                                                    <p>{channel.channelName}</p>
                                                </div>
                                                <button onClick={() => em.emitEvent("delete_channel", { channelID: channel.channelID })} style={{ background: "transparent", display: "flex", justifyContent: "center", alignItems: "center", border: "1px solid gray", borderRadius: 100, width: "12%", height: "100%" }}>
                                                    <svg xmlns="http://www.w3.org/2000/svg" width="17" height="17" fill="currentColor" className="bi bi-trash" viewBox="0 0 16 16">
                                                        <path d="M5.5 5.5A.5.5 0 0 1 6 6v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5m2.5 0a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5m3 .5a.5.5 0 0 0-1 0v6a.5.5 0 0 0 1 0z"/>
                                                        <path d="M14.5 3a1 1 0 0 1-1 1H13v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V4h-.5a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1H6a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1h3.5a1 1 0 0 1 1 1zM4.118 4 4 4.059V13a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V4.059L11.882 4zM2.5 3h11V2h-11z"/>
                                                    </svg>
                                                </button>
                                            </div>
                                        ))}
                                    </div>

                                    { selectedChannel && 
                                        <div>
                                            <div>
                                                <AtlasInput title="Channel Name" value={selectedChannel.channelName} onChange={(e) => setSelectedChannel(prev => ({
                                                    ...prev,
                                                    channelName: e.target.value
                                                }))}/>
                                            </div>

                                            <div>
                                                <button onClick={() => {
                                                    em.emitEvent("update_channel", { channel: selectedChannel })
                                                }}>Save</button>
                                                <button onClick={() => {
                                                    setSelectedChannel(server?.channels?.find(c => c.channelID === selectedChannel.channelID))
                                                }}>Cancel</button>
                                            </div>
                                        </div>
                                    }
                                </div>
                            </Tab>

                            <Tab className={styles.serverSettingsSettingsContent}>
                                <button onClick={() => create_server_invite_code(em, serverID)}>Create invite</button>

                                <div className={styles["codes-table"]}>
                                    <div className={styles["codes-table-header"]}>
                                        <span className={`${styles["table-column"]} ${styles["column-name"]} ${styles["column-short"]}`}></span>
                                        <span className={`${styles["table-column"]} ${styles["column-name"]}`}>Issued By</span>
                                        <span className={`${styles["table-column"]} ${styles["column-name"]}`}>Code</span>
                                        <span className={`${styles["table-column"]} ${styles["column-name"]}`}>Expires</span>
                                    </div>
                                    {serverCodes !== null && serverCodes !== undefined && serverCodes.map((code, index) => (
                                        <div key={index} className={styles["codes-table-list-item"]}>
                                            <span className={`${styles["table-column"]} ${styles["column-short"]}`}>
                                                <button className={styles["table-delete-button"]} onClick={() => em.emitEvent("delete_server_invite", { code: code.invite_code })}>
                                                    <svg xmlns="http://www.w3.org/2000/svg" width="35" height="35" fill="currentColor" className="bi bi-x" viewBox="0 0 16 16">
                                                        <path d="M4.646 4.646a.5.5 0 0 1 .708 0L8 7.293l2.646-2.647a.5.5 0 0 1 .708.708L8.707 8l2.647 2.646a.5.5 0 0 1-.708.708L8 8.707l-2.646 2.647a.5.5 0 0 1-.708-.708L7.293 8 4.646 5.354a.5.5 0 0 1 0-.708"/>
                                                    </svg>
                                                </button>
                                            </span>
                                            <span className={styles["table-column"]}>{code.issued_by.displayName}</span>
                                            <span className={styles["table-column"]}>{code.invite_code}</span>
                                            <span className={styles["table-column"]}>{code.expires}</span>
                                            <span className={`${styles["table-column"]} ${styles["column-short"]}`}>
                                                <button onClick={() => {
                                                    navigator.clipboard.writeText(`${globals.url_string.scheme}://${globals.url_string.subdomain}/invite?code=${code.invite_code}`);
                                                }} className={styles["table-share-button"]}>
                                                    <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" fill="currentColor" className="bi bi-share" viewBox="0 0 16 16">
                                                        <path d="M13.5 1a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3M11 2.5a2.5 2.5 0 1 1 .603 1.628l-6.718 3.12a2.5 2.5 0 0 1 0 1.504l6.718 3.12a2.5 2.5 0 1 1-.488.876l-6.718-3.12a2.5 2.5 0 1 1 0-3.256l6.718-3.12A2.5 2.5 0 0 1 11 2.5m-8.5 4a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3m11 5.5a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3"/>
                                                    </svg>
                                                </button>
                                            </span>
                                        </div>
                                    ))}
                                </div>
                            </Tab>

                            <Tab>
                                <p><strong>Users: {userList.offline.length + userList.online.length}</strong></p>
                                <br></br>
                                <div className={styles["members-list"]}>
                                    {[userList.offline, userList.online].map(content => (
                                        content.map(user => (
                                            <div key={user.userID} className={styles["members-list-user"]}>
                                                <div style={{ display: "flex", flexDirection: "row", alignItems: "center", gap: "15px" }}>
                                                    <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${user["picture"]}`} alt="" width={50} height={50} unoptimized quality={1} className={styles["avatar"]}/>
                                                    <div>
                                                        <p className={styles["members-list-display-name"]}>{user.displayName}</p>
                                                        {user.userID === server.owner.userID &&
                                                            <span>
                                                                <p>Owner</p>
                                                            </span>
                                                        }
                                                    </div>
                                                </div>

                                                {user.userID !== server.owner.userID &&
                                                    <div className={styles["members-list-user-options"]}>
                                                        <button onClick={() => kickUser(em, user.userID, "", serverID, "")}>Kick</button>
                                                        <button>Ban</button>
                                                        <button>Time</button>
                                                    </div>
                                                }
                                            </div>
                                        ))
                                    ))}
                                </div>
                            </Tab>
                        </Tabs>

                        <div>
                            <button onClick={() => setServerSettings(false)} className={styles["settings-close-button"]}>X</button>
                        </div>
                    </div>

                    <div className={styles["serverSettingsFooter"]}>
                        <button onClick={() => updateServer()}>Update</button>
                    </div>
                </div>
            }

            { channelPanel && <ChannelCreationPanel em={em} serverID={serverID} setChannelPanel={setChannelPanel}/> }


            <div className={styles.mainChat}>
                <div className={styles["server-actions-panel"]}>
                    <div className={styles["server-actions"]}>
                        <div>
                            <button onClick={() => leaveServer(em, token, serverID)}>Leave</button>
                            <button onClick={() => setServerSettings(true)}>
                                <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-sliders" viewBox="0 0 16 16">
                                    <path fillRule="evenodd" d="M11.5 2a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3M9.05 3a2.5 2.5 0 0 1 4.9 0H16v1h-2.05a2.5 2.5 0 0 1-4.9 0H0V3zM4.5 7a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3M2.05 8a2.5 2.5 0 0 1 4.9 0H16v1H6.95a2.5 2.5 0 0 1-4.9 0H0V8zm9.45 4a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3m-2.45 1a2.5 2.5 0 0 1 4.9 0H16v1h-2.05a2.5 2.5 0 0 1-4.9 0H0v-1z"/>
                                </svg>
                            </button>
                        </div>

                        <button onClick={() => setChannelPanel(true)}>+</button>
                    </div>
                    {server && server.channels && server.channels.map(channel => (
                        <button className={styles["channel-button"]} key={channel.channelID} onClick={() => router.push(`/bubble/server/${serverID}/${channel.channelID}`)}>{channel.channelName}</button>
                    ))}
                </div>

                <UserListProvider value={userList}>
                    {children}
                </UserListProvider>

                <div className={styles.userList}>
                    <strong>Online — {userList.online.length}</strong>
                    {userList.online.map((user, index) => (
                        <div key={index} className={styles.userListUser} onClick={() => setTargetUser(user)}>
                            <div style={{position: "relative"}}>
                                <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${user["picture"]}`} alt="" width={50} height={50} unoptimized quality={1}/>
                                <span className={`${styles.si} ${styles[user["status"]]}`}></span>
                            </div>
                            <div>
                                <strong>{user.displayName}</strong>
                                <p className={styles.userStatus}>{user.customStatus}</p>
                            </div>
                        </div>
                    ))}
                    <strong>Offline — {userList.offline.length}</strong>
                    {userList.offline.map((user, index) => (
                        <div key={index} className={styles.userListUser} onClick={() => setTargetUser(user)}>
                            <div style={{position: "relative"}}>
                                <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${user["picture"]}`} alt="" width={50} height={50} unoptimized quality={1}/>
                                <span className={`${styles.si} ${styles[user["status"]]}`}></span>
                            </div>
                            <div>
                                <strong>{user.displayName}</strong>
                                <p className={styles.userStatus}>{user.customStatus}</p>
                            </div>
                        </div>
                    ))}
                </div>

                {targetUser !== null && <UserPanel user={targetUser} setTargetUser={setTargetUser}/>}
            </div>
        </div>
    )
}