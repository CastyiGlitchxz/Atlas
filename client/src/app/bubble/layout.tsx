'use client'
import React, { createContext, Dispatch, SetStateAction, useEffect, useRef, useState } from "react";
import { Geist, Geist_Mono } from "next/font/google";
import { useRouter } from "next/navigation";
import { get_token } from "../../typescript/user";
import { construct_path } from "../../typescript/env";
import styles from "../../stylesheets/css/chat.module.css";
import { getWebSocket } from "../../typescript/websocket";
import { Profile, serverFormat } from "../../typescript/interfaces";
import { eventManager } from "../../typescript/eventsManager";
import { Tab, Tabs, UserPanel } from "../components";

type upt = {
    preview: Profile;
    setPreview: Dispatch<SetStateAction<Profile>>;
    setShowPreview: Dispatch<SetStateAction<string>>,
}

export const ProfilePanel = createContext<upt | undefined>(undefined);

const geistSans = Geist({
    variable: "--font-geist-sans",
    subsets: ["latin"],
});

const geistMono = Geist_Mono({
    variable: "--font-geist-mono",
    subsets: ["latin"],
});

function find_server(inviteCode: string) {
    const em = new eventManager();

    em.emitEvent("verify_invite", { "code": inviteCode });
};

function join_server(serverID: string) {
    const em = new eventManager();

    const token = get_token();
    em.emitEvent("join_server", { "token": token, sid: serverID });
};

export default function RootLayout({
    children,
}: Readonly<{
    children: React.ReactNode;
}>) {
    const emRef = useRef<eventManager | null>(null);

    if (!emRef.current) {
        emRef.current = new eventManager();
    }

    const em = emRef.current;

    const router = useRouter();
    const [selectedTab, setSelectedTab] = useState(0);
    const [serverList, setServerList] = useState<serverFormat[]>([]);
    const [promptVisibility, setPromptVisibility] = useState<string>("hidden");
    // const [promptTab, setPromptTab] = useState<string>("create");
    const [server, setServer] = useState<serverFormat>({
        name: "",
        owner: "",
        serverID: ""
    });
    const [preview, setPreview] = useState<Profile>({
        displayName: "",
        picture: "",
        userid: "",
        status: "offline",
        customStatus: "",
        bio: ""
    });
    const [showPreview, setShowPreview] = useState<string>("");

    function open_bubble() {
        router.push("/bubble");
    };

    function open_server(serverID: string) {
        router.push(`/bubble/server/${serverID}`);
    };

    function create_server() {
        const token = get_token();
        em.emitEvent("create_server", { "auth": token, server_name: server.name });
    };

    useEffect(() => {
        const token = get_token();
        em.emitEvent("update_status", { auth: token, status: "online" });
    }, []);

    useEffect(() => {
        const token = get_token();

        document.onfocus = function() {
            setTimeout(() => {
                em.emitEvent("update_status", { auth: token, status: "online" });
            }, 1000);
        };
        
        document.onblur = function() {
            setTimeout(() => {
                em.emitEvent("update_status", { auth: token, status: "idle" });
            }, 1000);
        };
    }, []);
    
    useEffect(() => {
        const token = get_token();

        window.onbeforeunload = function() {
            em.emitEvent("update_status", { auth: token, status: "offline" });
        }
    });

    useEffect(() => {
        (async () => {
            const token = get_token();

            const res = await fetch(construct_path("api/servers/get"), {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "Authorization": `Bearer ${token}`
                },
            });
            const data = await res.json();

            setServerList(data.servers);
        })();
    }, []);

    useEffect(() => {
        const ws = getWebSocket();

        const newNotification = (data) => {
            const notification = new Notification(`Message from ${data.sender.displayName}`, {
                body: data.sender.message,
                icon: data.sender.picture,
            });

            notification.onclick = function() {
                router.push(`/bubble/server/${data.serverID}`);
            };
        }

        const handler = (msg: MessageEvent) => {
            const {event, data} = JSON.parse(msg.data);
            
            const token = get_token();
            const server: serverFormat = data.server;

            switch(event) {
                case "invite":
                    if (!data.failed) {
                        join_server(data.server.sid);
                    } else {
                        console.log("failed");
                    }
                    break;

                case "creation_response":
                    join_server(data.server.serverID);
                    break;

                case "server_response":
                    setServerList(prev => [
                        ...prev,
                        server
                    ]);
                    break;

                case "notification":
                    // if (!data.sender.token || sameChat || document.hasFocus() && sameChat) break;
                    if (!data.sender || data.sender.token === token) break;
                    newNotification(data);
                    break;
                    
                    default:
                        break;
                    }
                }

        ws.addEventListener("message", handler);
        
        return () => {
            ws.removeEventListener("message", handler);
        };
    }, []);

    return (
        <ProfilePanel.Provider value={{ preview, setPreview, setShowPreview }}>
            <div className={`${geistSans.variable} ${geistMono.variable}`} style={{ display: "flex" }}>
                <div className={styles.sidebar}>
                    <div className={styles.serverIcon}>
                        <button onClick={() => open_bubble()} className={styles.serverIconPNG}>B</button>
                        <div className={styles.serverName}>
                            <p>My Bubble</p>
                        </div>
                    </div>
                    {serverList.map(server => (
                        <div key={server["serverID"]} className={styles.serverIcon}>
                            <button onClick={() => open_server(server["serverID"])} className={styles.serverIconPNG}>{server["name"][0]}</button>
                            <div className={styles.serverName}>
                                <p>{server["name"]}</p>
                            </div>
                        </div>
                    ))}
                    <div key={server["serverID"]} className={styles.serverIcon}>
                        <button onClick={() => setPromptVisibility("visible")} className={styles.newServer}>
                            <svg xmlns="http://www.w3.org/2000/svg" width="30" height="30" fill="currentColor" className="bi bi-patch-plus" viewBox="0 0 16 16">
                                <path fillRule="evenodd" d="M8 5.5a.5.5 0 0 1 .5.5v1.5H10a.5.5 0 0 1 0 1H8.5V10a.5.5 0 0 1-1 0V8.5H6a.5.5 0 0 1 0-1h1.5V6a.5.5 0 0 1 .5-.5"/>
                                <path d="m10.273 2.513-.921-.944.715-.698.622.637.89-.011a2.89 2.89 0 0 1 2.924 2.924l-.01.89.636.622a2.89 2.89 0 0 1 0 4.134l-.637.622.011.89a2.89 2.89 0 0 1-2.924 2.924l-.89-.01-.622.636a2.89 2.89 0 0 1-4.134 0l-.622-.637-.89.011a2.89 2.89 0 0 1-2.924-2.924l.01-.89-.636-.622a2.89 2.89 0 0 1 0-4.134l.637-.622-.011-.89a2.89 2.89 0 0 1 2.924-2.924l.89.01.622-.636a2.89 2.89 0 0 1 4.134 0l-.715.698a1.89 1.89 0 0 0-2.704 0l-.92.944-1.32-.016a1.89 1.89 0 0 0-1.911 1.912l.016 1.318-.944.921a1.89 1.89 0 0 0 0 2.704l.944.92-.016 1.32a1.89 1.89 0 0 0 1.912 1.911l1.318-.016.921.944a1.89 1.89 0 0 0 2.704 0l.92-.944 1.32.016a1.89 1.89 0 0 0 1.911-1.912l-.016-1.318.944-.921a1.89 1.89 0 0 0 0-2.704l-.944-.92.016-1.32a1.89 1.89 0 0 0-1.912-1.911z"/>
                            </svg>
                        </button>
                        
                        <div className={styles.serverName}>
                            <p>Server Hub</p>
                        </div>
                    </div>

                    <button onClick={() => router.push("/settings")} className={styles.settingsButton}>
                        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-sliders" viewBox="0 0 16 16">
                            <path fillRule="evenodd" d="M11.5 2a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3M9.05 3a2.5 2.5 0 0 1 4.9 0H16v1h-2.05a2.5 2.5 0 0 1-4.9 0H0V3zM4.5 7a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3M2.05 8a2.5 2.5 0 0 1 4.9 0H16v1H6.95a2.5 2.5 0 0 1-4.9 0H0V8zm9.45 4a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3m-2.45 1a2.5 2.5 0 0 1 4.9 0H16v1h-2.05a2.5 2.5 0 0 1-4.9 0H0v-1z"/>
                        </svg>
                        {/* Settings */}
                    </button>
                </div>

                {children}

                {promptVisibility === "visible" ? 
                    <div className={styles.exploreScreen}>
                        <div className={styles.serverHubHeader}>
                            <h2>Server Hub</h2>
                            <button onClick={() => setPromptVisibility("hidden")} className={styles.closeButton}>
                                <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" className="bi bi-x-circle" viewBox="0 0 16 16">
                                    <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14m0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16"/>
                                    <path d="M4.646 4.646a.5.5 0 0 1 .708 0L8 7.293l2.646-2.647a.5.5 0 0 1 .708.708L8.707 8l2.647 2.646a.5.5 0 0 1-.708.708L8 8.707l-2.646 2.647a.5.5 0 0 1-.708-.708L7.293 8 4.646 5.354a.5.5 0 0 1 0-.708"/>
                                </svg>
                            </button>
                        </div>
                        {/* <div className={styles.toggleButton}>
                            <button className={styles.selected} onClick={() => setPromptTab("create")}>Create</button>
                            <button onClick={() => setPromptTab("join")}>Join</button>
                        </div> */}

                        <div className={styles.serverHubContent}>
                            <span className={styles.modelLongButton} onClick={() => setSelectedTab(0)}>
                                <h4>Create Server</h4>
                                <p>Create your own server for a community or friends</p>
                            </span>

                            <span className={styles.modelLongButton} onClick={() => setSelectedTab(1)}>
                                <h4>Join Server</h4>
                                <p>Find random servers to join or provide an invite code</p>
                            </span>
                        </div>

                        <div className={styles.serverHubFooter}>

                        </div>

                        <Tabs selectedTab={selectedTab}>
                            <Tab>
                                <input type="text" placeholder="Give your server a name" value={server.name} onChange={(e) => setServer(prev => ({
                                    ...prev,
                                    name: e.target.value
                                }))}/>
                                <button onClick={() => create_server()}>Create Server</button>
                            </Tab>

                            <Tab>
                                <input type="text" placeholder="Enter server code" onKeyDown={(e) => {
                                    if (e.code === "Enter") {
                                        find_server(e.currentTarget.value);
                                    }
                                }}/>
                            </Tab>
                        </Tabs>
                    </div>
                    :
                    ""
                }

                {showPreview === "stack" &&
                    <UserPanel user={preview}/>
                }
            </div>
        </ProfilePanel.Provider>
    );
}
