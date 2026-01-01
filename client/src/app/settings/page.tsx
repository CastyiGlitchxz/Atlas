'use client'
import { useEffect, useRef, useState } from "react"
import { construct_path, globals } from "../../typescript/env";
import { get_token } from "../../typescript/user";
import styles from "../../stylesheets/css/settings.module.css";
import { Account, Profile } from "../../typescript/interfaces";
import { useRouter } from "next/navigation";
import { Tab, Tabs } from "../components";
import { getWebSocket } from "../../typescript/websocket";

export default function SettingsPage() {
    const router = useRouter();
    type User = Account & Profile;

    const [user, setUser] = useState<User>({
        displayName: "",
        userid: "",
        username: "",
        customStatus: "",
        picture: "",
        status: "offline",
        bio: ""
    });
    const [selectedTab, setSelectedTab] = useState(0);

    async function update_account() {
        const token = get_token();

        const res = await fetch(construct_path("api/account/update"), {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Authorization": `Bearer ${token}`
            },
            body: JSON.stringify({ "user": user }),
        });
        const data = await res.json();
        console.log(data);
    }

    useEffect(() => {
        async function getUserData() {
            const token = get_token();

            const res = await fetch(construct_path("api/account/get"), {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "Authorization": `Bearer ${token}`
                },
                body: JSON.stringify({  })
            });
            const data = await res.json();
            setUser(data.user);
        }
        getUserData();
    }, []);

    function enableNotifications() {
        if (!("Notification" in window)) {
            alert("This browser does not support desktop notification");
        } else if (Notification.permission === "granted") {
            new Notification("Hi there!");
        } else if (Notification.permission !== "denied") {
            Notification.requestPermission().then((permission) => {
            if (permission === "granted") {
                new Notification("Hi there!");
            }
            });
        };
    };

    const wsRef = useRef<WebSocket | null>(null);

    const connectWS = () => {
        const ws = getWebSocket();
        if (wsRef.current) return;
        ws.binaryType = "arraybuffer"; // important for binary
        ws.onopen = () => console.log("WebSocket connected");
        ws.onmessage = (msg) => console.log("Server says:", msg.data);
        wsRef.current = ws;
    };

    useEffect(() => {
        connectWS();
    });

    const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file || !wsRef.current || wsRef.current.readyState !== WebSocket.OPEN) return;

        // send raw binary
        file.arrayBuffer().then((buffer) => {
            // wsRef.current!.send(buffer);
            // console.log("Sent profile image:", file.name, "Size:", buffer.byteLength);
        });
    };

    return (
        <div className={styles.main}>
            <div className={styles.settingBar}>
                <button onClick={() => router.push("/bubble")}>
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-arrow-left" viewBox="0 0 16 16">
                        <path fillRule="evenodd" d="M15 8a.5.5 0 0 0-.5-.5H2.707l3.147-3.146a.5.5 0 1 0-.708-.708l-4 4a.5.5 0 0 0 0 .708l4 4a.5.5 0 0 0 .708-.708L2.707 8.5H14.5A.5.5 0 0 0 15 8"/>
                    </svg>
                    Back
                </button>

                <hr style={{ borderColor: "#1c1c1c" }}/>

                <button onClick={() => setSelectedTab(0)}>
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-person-lines-fill" viewBox="0 0 16 16">
                        <path d="M6 8a3 3 0 1 0 0-6 3 3 0 0 0 0 6m-5 6s-1 0-1-1 1-4 6-4 6 3 6 4-1 1-1 1zM11 3.5a.5.5 0 0 1 .5-.5h4a.5.5 0 0 1 0 1h-4a.5.5 0 0 1-.5-.5m.5 2.5a.5.5 0 0 0 0 1h4a.5.5 0 0 0 0-1zm2 3a.5.5 0 0 0 0 1h2a.5.5 0 0 0 0-1zm0 3a.5.5 0 0 0 0 1h2a.5.5 0 0 0 0-1z"/>
                    </svg>
                    Profile
                </button>

                <button onClick={() => setSelectedTab(1)}>
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-palette2" viewBox="0 0 16 16">
                        <path d="M0 .5A.5.5 0 0 1 .5 0h5a.5.5 0 0 1 .5.5v5.277l4.147-4.131a.5.5 0 0 1 .707 0l3.535 3.536a.5.5 0 0 1 0 .708L10.261 10H15.5a.5.5 0 0 1 .5.5v5a.5.5 0 0 1-.5.5H3a3 3 0 0 1-2.121-.879A3 3 0 0 1 0 13.044m6-.21 7.328-7.3-2.829-2.828L6 7.188zM4.5 13a1.5 1.5 0 1 0-3 0 1.5 1.5 0 0 0 3 0M15 15v-4H9.258l-4.015 4zM0 .5v12.495zm0 12.495V13z"/>
                    </svg>
                    Appearance
                </button>

                <button onClick={() => setSelectedTab(2)}>
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-app-indicator" viewBox="0 0 16 16">
                        <path d="M5.5 2A3.5 3.5 0 0 0 2 5.5v5A3.5 3.5 0 0 0 5.5 14h5a3.5 3.5 0 0 0 3.5-3.5V8a.5.5 0 0 1 1 0v2.5a4.5 4.5 0 0 1-4.5 4.5h-5A4.5 4.5 0 0 1 1 10.5v-5A4.5 4.5 0 0 1 5.5 1H8a.5.5 0 0 1 0 1z"/>
                        <path d="M16 3a3 3 0 1 1-6 0 3 3 0 0 1 6 0"/>
                    </svg>
                    Notifications
                </button>

                <button onClick={() => router.replace("/logout")} className={styles.logoutButton}>
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-box-arrow-left" viewBox="0 0 16 16">
                        <path fillRule="evenodd" d="M6 12.5a.5.5 0 0 0 .5.5h8a.5.5 0 0 0 .5-.5v-9a.5.5 0 0 0-.5-.5h-8a.5.5 0 0 0-.5.5v2a.5.5 0 0 1-1 0v-2A1.5 1.5 0 0 1 6.5 2h8A1.5 1.5 0 0 1 16 3.5v9a1.5 1.5 0 0 1-1.5 1.5h-8A1.5 1.5 0 0 1 5 12.5v-2a.5.5 0 0 1 1 0z"/>
                        <path fillRule="evenodd" d="M.146 8.354a.5.5 0 0 1 0-.708l3-3a.5.5 0 1 1 .708.708L1.707 7.5H10.5a.5.5 0 0 1 0 1H1.707l2.147 2.146a.5.5 0 0 1-.708.708z"/>
                    </svg>
                    Logout
                </button>
            </div>

            <div className={styles.settings}>
                <Tabs selectedTab={selectedTab}>
                    <Tab>
                        <div className={styles.userInfo}>
                            <label htmlFor="pfp_change">
                                {/* eslint-disable-next-line @next/next/no-img-element */}
                                <img src={user.picture !== "" ? user.picture : "/assets/profile.jpg"} alt="e" width={100} height={100} className={styles.profilePicture}/>
                            </label>
                            <div className={styles.subInfo}>
                                <div className={styles.userNames}>
                                    <input value={user.username} onChange={(e) => setUser(prev => ({
                                        ...prev,
                                        username: e.target.value
                                    }))} type="text"/>
                                    <p>*</p>
                                    <input value={user.displayName} onChange={(e) => setUser(prev => ({
                                        ...prev,
                                        displayName: e.target.value
                                    }))} type="text"/>
                                </div>

                                <input value={user.customStatus} onChange={(e) => setUser(prev => ({
                                    ...prev,
                                    customStatus: e.target.value
                                }))} type="text"/>
                            </div>
                        </div>

                        <input type="file" accept="image/*" onChange={handleFileChange} id="pfp_change" hidden/>

                        <textarea style={{resize: "none"}} rows={10} value={user.bio} onChange={(e) => setUser(prev => ({
                            ...prev,
                            bio: e.target.value
                        }))}/>
                    </Tab>

                    <Tab>
                        <h1>Appearance</h1>
                    </Tab>

                    <Tab>
                        <button onClick={() => enableNotifications()}>Enable Notifications</button>
                    </Tab>
                </Tabs>

                {/* <input value={user.picture}onChange={(e) => setUser(prev => ({
                    ...prev,
                    picture: e.target.value
                }))} type="text"/> */}

                <button onClick={() => update_account()}>Update</button>
            </div>

        </div>
    )
}