import { Friend, Profile } from "../typescript/interfaces";
import styles from "../stylesheets/css/components.module.css";
import React, { CSSProperties, Dispatch, ReactNode, SetStateAction, useState } from "react";
import { globals } from "../typescript/env";

export function UserPanel({ user, setTargetUser }: { user: (Profile | Friend), setTargetUser: Dispatch<SetStateAction<Profile | null | Friend>> }) {
    const statusMap = {
        online: "lime",
        offline: "gray",
        idle: "rgb(255, 187, 0)",
    };

    return (
        <div className={styles.userPanel}>
            <div style={{ background: `url('${globals.url_string.scheme}://${globals.url_string.subdomain}${user.picture}')`, width: "100%", height: "12rem", display: "grid", gap: 10, borderTopLeftRadius: "inherit",
    borderTopRightRadius: "inherit" }}>
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

                <div style={{ display: "flex", gap: 10, alignItems: "center" }}>
                    <img src={`${globals.url_string.scheme}://${globals.url_string.subdomain}${user?.picture}`} width={110} height={110} alt="" />
                    <div className={styles["user-content"]}>
                        <span className={styles["user-username"]}>
                            {user.displayName}
                        </span>
                        <span className={styles["user-custom-status"]}>{user.customStatus}</span>
                    </div>
                </div>
            </div>

            <div style={{ display: "flex", flexDirection: "row", alignItems: "center", gap: "15px" }}>
                <div className={styles["user-buttons"]}>
                    <button>
                        <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" fill="currentColor" className="bi bi-person-plus" viewBox="0 0 16 16">
                            <path d="M6 8a3 3 0 1 0 0-6 3 3 0 0 0 0 6m2-3a2 2 0 1 1-4 0 2 2 0 0 1 4 0m4 8c0 1-1 1-1 1H1s-1 0-1-1 1-4 6-4 6 3 6 4m-1-.004c-.001-.246-.154-.986-.832-1.664C9.516 10.68 8.289 10 6 10s-3.516.68-4.168 1.332c-.678.678-.83 1.418-.832 1.664z"/>
                            <path fillRule="evenodd" d="M13.5 5a.5.5 0 0 1 .5.5V7h1.5a.5.5 0 0 1 0 1H14v1.5a.5.5 0 0 1-1 0V8h-1.5a.5.5 0 0 1 0-1H13V5.5a.5.5 0 0 1 .5-.5"/>
                        </svg>
                        <p>Add Friend</p>
                    </button>

                    <button>
                        <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" fill="currentColor" className="bi bi-chat" viewBox="0 0 16 16">
                            <path d="M2.678 11.894a1 1 0 0 1 .287.801 11 11 0 0 1-.398 2c1.395-.323 2.247-.697 2.634-.893a1 1 0 0 1 .71-.074A8 8 0 0 0 8 14c3.996 0 7-2.807 7-6s-3.004-6-7-6-7 2.808-7 6c0 1.468.617 2.83 1.678 3.894m-.493 3.905a22 22 0 0 1-.713.129c-.2.032-.352-.176-.273-.362a10 10 0 0 0 .244-.637l.003-.01c.248-.72.45-1.548.524-2.319C.743 11.37 0 9.76 0 8c0-3.866 3.582-7 8-7s8 3.134 8 7-3.582 7-8 7a9 9 0 0 1-2.347-.306c-.52.263-1.639.742-3.468 1.105"/>
                        </svg>
                    </button>

                    <button>
                        <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" fill="currentColor" className="bi bi-telephone" viewBox="0 0 16 16">
                            <path d="M3.654 1.328a.678.678 0 0 0-1.015-.063L1.605 2.3c-.483.484-.661 1.169-.45 1.77a17.6 17.6 0 0 0 4.168 6.608 17.6 17.6 0 0 0 6.608 4.168c.601.211 1.286.033 1.77-.45l1.034-1.034a.678.678 0 0 0-.063-1.015l-2.307-1.794a.68.68 0 0 0-.58-.122l-2.19.547a1.75 1.75 0 0 1-1.657-.459L5.482 8.062a1.75 1.75 0 0 1-.46-1.657l.548-2.19a.68.68 0 0 0-.122-.58zM1.884.511a1.745 1.745 0 0 1 2.612.163L6.29 2.98c.329.423.445.974.315 1.494l-.547 2.19a.68.68 0 0 0 .178.643l2.457 2.457a.68.68 0 0 0 .644.178l2.189-.547a1.75 1.75 0 0 1 1.494.315l2.306 1.794c.829.645.905 1.87.163 2.611l-1.034 1.034c-.74.74-1.846 1.065-2.877.702a18.6 18.6 0 0 1-7.01-4.42 18.6 18.6 0 0 1-4.42-7.009c-.362-1.03-.037-2.137.703-2.877z"/>
                        </svg>
                    </button>
                </div>

                {user.status !== "offline" && <hr style={{ border: "none", height: "100%", width: "2px", backgroundColor: "gray" }}></hr>}

                {user.status !== "offline" && 
                    <span className={styles["client-display"]}>
                        <p title={user.client && user.client.internalSlug ? user.client.internalSlug : "unknown-client"}>{user.client && user.client.clientName ? user.client.clientName : "Unknown Client"}</p>
                    </span>
                }
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
    className
}: {
    children: ReactNode[];
    selectedTab: number;
    className?: string;
}) {
    // const [selectedTab, setSelectedTab] = useState(0);

    return (
        <div className={className}>
            {/* {children.map((item, index) => (
                <TabTitle title={item.props.title} key={index} index={index} setSelectedTab={setSelectedTab}/>
            ))} */}

            {children && children[selectedTab]}
        </div>
    );
}

export function Tab({ children, className, style }: { children: ReactNode, className?: string | undefined, style?: CSSProperties | undefined }) {
    return <div className={className} style={style}>{children}</div>;
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

export function AtlasInput({ title, onChange, value, readonly, onKeyDown }: { title?: string, onChange?: React.ChangeEventHandler<HTMLInputElement>, value?: string | number | readonly string[], readonly?: boolean, onKeyDown?: React.KeyboardEventHandler<HTMLInputElement> }) {
    return (
        <div className={styles.spCustomInput}>
            <p style={{ color: "gray", fontSize: "12px", width: "fit-content" }}>{title}</p>
            <input value={value} onChange={onChange} type="text" style={{ fontSize: "17px" }} readOnly={readonly} onKeyDown={onKeyDown}/>
        </div>
    )
}