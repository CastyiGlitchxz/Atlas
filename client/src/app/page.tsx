"use client";

import React, { useState, useEffect, Dispatch, SetStateAction } from "react";
import { useRouter } from "next/navigation";
import styles from "../stylesheets/css/login.module.css";
import { construct_path } from "../typescript/env";
import { login_status } from "../typescript/user";
import { loginResponse } from "../typescript/interfaces";
import { FloatingInput, Tab, Tabs } from "./components";

function LoginForm({ setForm }: { setForm: Dispatch<SetStateAction<string>> }) {
    const router = useRouter();
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");
    const [status, setStatus] = useState("");
    const [loading, setLoading] = useState(false);

    async function login(e: React.FormEvent) {
        e.preventDefault();
        setStatus("");
        setLoading(true);

        try {
            const res = await fetch(construct_path("api/login"), {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ username, password }),
            });

            const data: loginResponse = await res.json();

            if (!res.ok) {
                setStatus(data.error ?? "Invalid username or password.");
                return;
            }

            localStorage.setItem("token", data.response.token);
            router.replace("/bubble");
        } catch (err) {
            setStatus("Network error. Please try again.");
        } finally {
            setLoading(false);
        }
    }

    useEffect(() => {
        login_status().then((loggedIn) => {
            if (loggedIn) router.replace("/bubble");
        });
    }, [router]);

    return (
        <div className={styles.formContainer}>
            <div className={styles.headerLogin}>
                <div className={styles.brandSection}>
                    <h3>Atlas</h3>
                    <p>Welcome back</p>
                </div>
                <button onClick={() => setForm("creation")} className={styles.formButton}>
                    Create Account
                </button>
            </div>

            <div className={styles.loginForm}>
                <h1>Login</h1>
                <form onSubmit={login} className={styles.stack}>
                    <FloatingInput label="Username" value={username} onChange={(v) => { setUsername(v); setStatus(""); }} />
                    <FloatingInput type="password" label="Password" value={password} onChange={(v) => { setPassword(v); setStatus(""); }} />
                    <button type="submit" className={styles.loginButton} disabled={loading}>
                        {loading ? "Logging in..." : "Login"}
                    </button>
                </form>
                {status && (
                    <div className={styles.status} aria-live="polite">
                        <p className={styles.icon}>!</p>
                        <p className={styles.details}>{status}</p>
                    </div>
                )}
            </div>
        </div>
    );
}

function CreationForm({ setForm }: { setForm: Dispatch<SetStateAction<string>> }) {
    const [username, setUsername] = useState("");
    const [displayName, setDisplayName] = useState("");
    const [password, setPassword] = useState("");
    const [bio, setBio] = useState<string>("");
    const [customStatus, setCustomStatus] = useState<string>("");
    const [status, setStatus] = useState("");
    const [loading, setLoading] = useState(false);
    const [tab, setTab] = useState<number>(0);

    async function createAccount(e: React.FormEvent) {
        e.preventDefault();
        setStatus("");
        setLoading(true);

        try {
            const res = await fetch(construct_path("api/create"), {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ username, password, displayName, customStatus, bio }),
            });
            const data = await res.json();

            if (!res.ok) {
                setStatus("Account creation failed.");
                return;
            }

            setForm("login");
        } catch (err) {
            setStatus("Network error. Please try again.");
        } finally {
            setLoading(false);
        }
    }

    return (
        <div className={styles.formContainer}>
            <div className={styles.headerLogin}>
                <div className={styles.brandSection}>
                    <h3>Atlas</h3>
                    <p>Get started</p>
                </div>
                <button onClick={() => setForm("login")} className={styles.formButton}>
                    Login
                </button>
            </div>

            <div className={styles.loginForm}>
                <h1>Create Account</h1>
                <form onSubmit={createAccount} className={styles.stack}>
                    <Tabs selectedTab={tab}>
                        <Tab>
                            <FloatingInput label="Username" value={username} onChange={setUsername} />
                            <FloatingInput label="Display Name" value={displayName} onChange={setDisplayName} />
                            <FloatingInput type="Password" label="Password" value={password} onChange={setPassword} />
                            <FloatingInput label="Custom Status" value={customStatus} onChange={setCustomStatus} />
                            <textarea rows={10} cols={15} placeholder="Enter something about yourself" className={styles.aboutPanel} onChange={(e) => setBio(e.target.value)}></textarea>
                        </Tab>

                        <Tab>
                            <h1></h1>
                        </Tab>
                    </Tabs>

                    <button type="submit" className={styles.loginButton} disabled={loading}>
                        {loading ? "Creating..." : "Create Account"}
                    </button>
                </form>

                {status && (
                    <div className={styles.status} aria-live="polite">
                        <p className={styles.icon}>!</p>
                        <p className={styles.details}>{status}</p>
                    </div>
                )}
            </div>
        </div>
    );
}

export default function Home() {
    const [form, setForm] = useState("login");

    return (
        <div className={styles.main}>
            <div className={styles.stars}>
                <div className={styles.asteroid}></div>
                <div className={styles.asteroid}></div>
                <div className={styles.asteroid}></div>
                <div className={styles.asteroid}></div>
                <div className={styles.asteroid}></div>
            </div>
            <div className={styles.cardWrapper}>
                {form === "login" ? <LoginForm key="login" setForm={setForm} /> : <CreationForm key="create" setForm={setForm} />}
            </div>
        </div>
    );
}
