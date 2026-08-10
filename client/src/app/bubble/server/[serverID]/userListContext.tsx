'use client';
import React, { createContext, useContext } from 'react';
import { Profile } from '../../../../typescript/interfaces';

interface UserList {
    online: Profile[];
    offline: Profile[];
}

const UserListContext = createContext<UserList | null>(null);

export function UserListProvider({ children, value }: { children: React.ReactNode; value: UserList }) {
    return (
        <UserListContext.Provider value={value}>
            {children}
        </UserListContext.Provider>
    );
}

export const useUserList = () => {
    const context = useContext(UserListContext);
    if (!context) {
        throw new Error("useUserList must be used within a UserListProvider");
    }
    return context;
};