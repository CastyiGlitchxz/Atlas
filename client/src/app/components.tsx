import { Profile } from "../typescript/interfaces";
import styles from "../stylesheets/css/components.module.css";
import React, { Dispatch, ReactNode, SetStateAction, useState } from "react";
import { globals } from "../typescript/env";

export function UserPanel({ user, setTargetUser }: { user: Profile, setTargetUser: Dispatch<SetStateAction<Profile | null>> }) {
    const statusMap = {
        online: "lime",
        offline: "gray",
        idle: "rgb(255, 187, 0)",
    };

    return (
        <div className={styles.userPanel}>
            <div style={{ background: `url('${user.picture}')`, width: "100%", height: "12rem", display: "grid", gap: 10 }}>
                <div className={styles.userPanelHeader}>
                    <button
                        onClick={() => setTargetUser(null)} 
                        className={styles.panelUserCloseButton}
                    >
                        <svg
                            xmlns="http://www.w3.org/2000/svg"
                            width="23"
                            height="23"
                            fill="currentColor"
                            className="bi bi-x-diamond-fill"
                            viewBox="0 0 16 16"
                        >
                            <path d="M9.05.435c-.58-.58-1.52-.58-2.1 0L4.047 3.339 8 7.293l3.954-3.954L9.049.435zm3.61 3.611L8.708 8l3.954 3.954 2.904-2.905c.58-.58.58-1.519 0-2.098l-2.904-2.905zm-.706 8.614L8 8.708l-3.954 3.954 2.905 2.904c.58.58 1.519.58 2.098 0l2.905-2.904zm-8.614-.706L7.292 8 3.339 4.046.435 6.951c-.58.58-.58 1.519 0 2.098z" />
                        </svg>
                    </button>

                    <div
                        className={styles.panelUserStatus}
                        style={{ borderColor: statusMap[user.status] }}
                    >
                        <span style={{ background: statusMap[user.status] }}></span>
                        <p>{user.status}</p>
                    </div>
                </div>

                <div style={{ display: "flex", gap: 10 }}>
                    <img src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${user?.picture}`} width={110} height={110} alt="" />
                    <div style={{ display: "flex", flexDirection: "column", gap: 3 }}>
                        <span
                            style={{ fontSize: "30px" }}
                            className={styles.userQC}
                        >
                            {user.displayName}
                        </span>
                        <span className={styles.userQC}>{user.customStatus}</span>
                    </div>
                </div>
            </div>

            <div className={styles.userButtons}>
                <button>Add Friend</button>
                <button>Message</button>
                <button>Call</button>
            </div>

            <textarea
                value={user.bio}
                className={styles.bioPanel}
                rows={10}
                cols={50}
                readOnly
            ></textarea>

            <div className={styles.userRibbon}>
                <button>
                    <svg
                        xmlns="http://www.w3.org/2000/svg"
                        width="16"
                        height="16"
                        fill="currentColor"
                        className="bi bi-person-circle"
                        viewBox="0 0 16 16"
                    >
                        <path d="M11 6a3 3 0 1 1-6 0 3 3 0 0 1 6 0" />
                        <path
                            fillRule="evenodd"
                            d="M0 8a8 8 0 1 1 16 0A8 8 0 0 1 0 8m8-7a7 7 0 0 0-5.468 11.37C3.242 11.226 4.805 10 8 10s4.757 1.225 5.468 2.37A7 7 0 0 0 8 1"
                        />
                    </svg>
                    Profile
                </button>

                <button>
                    <svg
                        xmlns="http://www.w3.org/2000/svg"
                        width="16"
                        height="16"
                        fill="currentColor"
                        className="bi bi-award"
                        viewBox="0 0 16 16"
                    >
                        <path d="M9.669.864 8 0 6.331.864l-1.858.282-.842 1.68-1.337 1.32L2.6 6l-.306 1.854 1.337 1.32.842 1.68 1.858.282L8 12l1.669-.864 1.858-.282.842-1.68 1.337-1.32L13.4 6l.306-1.854-1.337-1.32-.842-1.68zm1.196 1.193.684 1.365 1.086 1.072L12.387 6l.248 1.506-1.086 1.072-.684 1.365-1.51.229L8 10.874l-1.355-.702-1.51-.229-.684-1.365-1.086-1.072L3.614 6l-.25-1.506 1.087-1.072.684-1.365 1.51-.229L8 1.126l1.356.702z" />
                        <path d="M4 11.794V16l4-1 4 1v-4.206l-2.018.306L8 13.126 6.018 12.1z" />
                    </svg>
                    Badges
                </button>

                <button>
                    <svg
                        xmlns="http://www.w3.org/2000/svg"
                        width="16"
                        height="16"
                        fill="currentColor"
                        className="bi bi-arrow-left-right"
                        viewBox="0 0 16 16"
                    >
                        <path
                            fillRule="evenodd"
                            d="M1 11.5a.5.5 0 0 0 .5.5h11.793l-3.147 3.146a.5.5 0 0 0 .708.708l4-4a.5.5 0 0 0 0-.708l-4-4a.5.5 0 0 0-.708.708L13.293 11H1.5a.5.5 0 0 0-.5.5m14-7a.5.5 0 0 1-.5.5H2.707l3.147 3.146a.5.5 0 1 1-.708.708l-4-4a.5.5 0 0 1 0-.708l4-4a.5.5 0 1 1 .708.708L2.707 4H14.5a.5.5 0 0 1 .5.5"
                        />
                    </svg>
                    Related
                </button>
            </div>
        </div>
    );
}

export function TabTitle({
    title,
    setSelectedTab,
    index,
}: {
    title: string;
    setSelectedTab: Dispatch<SetStateAction<number>>;
    index: number;
}) {
    return <button onClick={() => setSelectedTab(index)}>{title}</button>;
}

export function Tabs({
    children,
    selectedTab,
}: {
    children: ReactNode[];
    selectedTab: number;
}) {
    // const [selectedTab, setSelectedTab] = useState(0);

    return (
        <div>
            {/* {children.map((item, index) => (
                <TabTitle title={item.props.title} key={index} index={index} setSelectedTab={setSelectedTab}/>
            ))} */}

            {children && children[selectedTab]}
        </div>
    );
}

export function Tab({ children }: { children: ReactNode }) {
    return <div>{children}</div>;
}

export function FloatingInput({
    type = "text",
    value,
    label,
    onChange,
}: {
    type?: string;
    value: string;
    label: string;
    onChange: (v: string) => void;
}) {
    const [focused, setFocused] = useState(false);

    return (
        <div className={`${styles.inputGroup} ${focused || value ? styles.active : ""}`}>
            <label className={styles.label}>{label}</label>
            <input
                type={type}
                value={value}
                required
                onFocus={() => setFocused(true)}
                onBlur={() => setFocused(false)}
                autoComplete={type === "password" ? "current-password" : "username"}
                onChange={(e) => onChange(e.target.value)}
                className={styles.inputField}
            />
        </div>
    );
}

export function AtlasInput({ title, onChange, value, readonly, onKeyDown }: { title?: string, onChange?: any, value?: any, readonly?: boolean, onKeyDown?: any }) {
    return (
        <div className={styles.spCustomInput}>
            <p style={{ color: "gray", fontSize: "12px", width: "fit-content" }}>{title}</p>
            <input value={value} onChange={onChange} type="text" style={{ fontSize: "17px" }} readOnly={readonly} onKeyDown={onKeyDown}/>
        </div>
    )
}