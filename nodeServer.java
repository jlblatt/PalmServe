////////////////////////////////////////////////////////////////////////////////
//  nodeServer.java                                                           //
////////////////////////////////////////////////////////////////////////////////

import java.awt.event.*;
import java.awt.*;
import java.io.*;
import java.net.*;
import java.util.*;
import javax.swing.*;
import javax.swing.event.*;

/////////////////////////////////////////////////
//
//  PUBLIC CLASS nodeServer
//  
//  Responsible for listening for connections and running userThreads
//
/////////////////////////////////////////////////

public class nodeServer {
    
    private final int PORT = 2600;
    private final int MAX_USERS = 10;
    private int numUsers = 0;
    private userThread[] users = new userThread[MAX_USERS];
    
    public nodeServer() {
        
        //Run GUI as a thread
        javax.swing.SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                serverGUI mainWindow = new serverGUI();
                mainWindow.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            }
        } );
        
        
        //Main server loop- accept connections and dispatch threads
        try {
            ServerSocket s = new ServerSocket(PORT);
            while(true) {
                try {
                    Socket nextSocket = s.accept();
                    synchronized(this) {
                        users[numUsers] = new userThread(nextSocket, numUsers);
                        users[numUsers].start();
                        numUsers++;
                    }
                }
                catch(Exception e) {
                    System.out.println("Error - Connection unsucessful: " + e.toString());
                }
            }  //while
        }
        catch(Exception e) {
            System.out.println("Error - Server malfunction, shutting down: " + e.toString());
            System.exit(-1);
        }
        
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    /////////////////////////////////////////////////
    //
    //  PUBLIC CLASS serverGUI EXTENDS JFrame
    //
    //  Draws the GUI and communicates with the userThreads
    //
    /////////////////////////////////////////////////
    
    public class serverGUI extends JFrame {
        
        private final int WINX = 800 , WINY = 900;
        
        private String motd;
        private String serverName = "helloServer";
        private LinkedList itemList = new LinkedList();
        private LinkedList orderList = new LinkedList();
        private LinkedList inbox = new LinkedList();
        private int nextOrderNum = 100;
        
        //GUI objects
        JPanel titlePanel, orderPanel, buttonPanel;
        JButton msgButton, brdcstButton, motdButton, inboxButton;
        JLabel usersLabel, logoLabel;
        ImageIcon logo;
        
        
        public serverGUI() {
            super("Service GUI");
            final Container cont = getContentPane();
            cont.setLayout(new BorderLayout(5,5));
            
            //Graphic
            try {
                logo = new ImageIcon(new URL("http://www.pitt.edu/~jlb213/splash.jpg"));
            }
            catch(MalformedURLException e) {logo = new ImageIcon();}
            logoLabel = new JLabel(logo);
            
            
            //Panels
            titlePanel = new JPanel(new BorderLayout(5,5));
            orderPanel = new JPanel(new GridLayout(2,4,5,5));
            buttonPanel = new JPanel(new GridLayout(1,4,5,5));
            
            //Buttons
            msgButton = new JButton("Message User...");
            brdcstButton = new JButton("Broadcast Message...");
            motdButton = new JButton("Set Message of the Day...");
            inboxButton = new JButton("Check Messages...");
            msgButton.addActionListener(new ButtonHandler());
            brdcstButton.addActionListener(new ButtonHandler());
            motdButton.addActionListener(new ButtonHandler());
            inboxButton.addActionListener(new ButtonHandler());
            
            //Labels
            usersLabel = new JLabel("Users connected: ");
            usersLabel.setHorizontalAlignment(JLabel.CENTER);
            
            buttonPanel.add(msgButton);
            buttonPanel.add(inboxButton);
            buttonPanel.add(brdcstButton);
            buttonPanel.add(motdButton);
            
            titlePanel.add(usersLabel, BorderLayout.SOUTH);
            titlePanel.add(logoLabel, BorderLayout.CENTER);
            cont.add(titlePanel, BorderLayout.NORTH);
            cont.add(orderPanel, BorderLayout.CENTER);
            cont.add(buttonPanel, BorderLayout.SOUTH);
            
            setSize(WINX,WINY);
            setVisible(true);
            
            setUpItems();
            
            ///////////////////////////////////////////////////
            //Set up timer to check for changes
            //This is where periodically executed code will go
            //////////////////////////////////////////////////
            Action updateGUI = new AbstractAction() {
                public void actionPerformed(ActionEvent e) {
                    updateUsersLabel();
                    updateOrders();
                    processMessages();
		    cont.paintAll(cont.getGraphics());
                }
            };
            
            javax.swing.Timer t = new javax.swing.Timer(1000,updateGUI);
            t.start();
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        ////
        //Action Handlers
        ////
        
        
        //Button Handler - handle clicks on lower buttons by opening new windows
        private class ButtonHandler implements ActionListener {
        
            public void actionPerformed(ActionEvent e) {
                String whichButton = e.getActionCommand();
                
                if(whichButton.equals("Message User...")){
                    msgDialog sendMsg = new msgDialog();
                }
                
                else if(whichButton.equals("Broadcast Message...")) {
                    brdcstDialog brdcst = new brdcstDialog();
                }
                
                else if(whichButton.equals("Set Message of the Day...")) {
                    motdDialog setMOTD = new motdDialog();
                }
                
                else if(whichButton.equals("Check Messages...")) {
                    inboxDialog checkMsgs = new inboxDialog();
                }
                else;
                
            }
            
        }
        
        
        
        
        
        //Order Handler - close out orders when clicked
         private class OrderHandler implements ActionListener {
        
            public void actionPerformed(ActionEvent e) {
                String whichOrder = e.getActionCommand();
                int i = new Integer(whichOrder.substring(whichOrder.length()-1)).intValue();
                orderList.remove(i);
            }
            
            
        }
        
        
        
        
        
        
        
        
        
        
        
        ////
        //Utilility and periodic functions
        ////
        
        
        
        //Set up available items array
        private void setUpItems() {

            itemList.add(new Item(
                1001,
                "Pitt Burger",
                "Delicious sandwhich made from panther meat.  Comes with fires.",
                6.99));
            
            itemList.add(new Item(
                1002,
                "Engineer's Platter",
                "Large pizza and a two-liter of Jolt cola.",
                9.99));
            
            itemList.add(new Item(
                1003,
                "Chicken Sandwich",
                "Grilled chicken breast, lettuce, tomatoes, and mayo.",
                5.99));
            
            itemList.add(new Item(
                1004,
                "Pittsburgh Hero",
                "Fries and cole slaw on a sub.  Comes with ranch dressing.",
                3.49));
            
            itemList.add(new Item(
                1005,
                "Fruit Cup",
                "Assortment of macintosh, tiger, and lisa apples.",
                1.99));
            
            itemList.add(new Item(
                1006,
                "Benedum Special Stout",
                "A special dark beer brewed in the basement of Benedum.",
                2.99));
            
            itemList.add(new Item(
                1007,
                "Fountain Drink",
                "Any soda - Coke, Diet Coke, Sprite, Mountain Dew, Jolt.",
                1.09));

        }
        
        
        //Check what users are connected and display them
        private void updateUsersLabel() {
            String s = "  ";
            for(int i = 0 ; i < numUsers ; i++) {
                s = s + users[i].userName + ", ";
            }
            usersLabel.setText("Connected: " + s.substring(0,s.length()-2));
        }
        
        
        
        //Update orders panel
        private void updateOrders() {
            orderPanel.removeAll();
            
            int end = orderList.size();
            if(end > 8) end = 8;
            for(int i = 0; i < end ; i++) {
                Order o = (Order)orderList.get(i);
                String infoString = "(" + o.id + ") " + o.user + " #" + o.ref;
                String itemString = "";
                
                for(int j = 0 ; j < o.size() ; j++) {
                    Item item = (Item)o.get(j);
                    itemString = itemString + ">" + item.name + " : " + item.mods + "\n";
                }
                
                if(o.closed == true) itemString = itemString + "***ORDER COMPLETE***\n";
                if(findUserByName(o.user) == -1) itemString = itemString + "***USER DISCONNECTED***\n";
                
                JPanel p = new JPanel(new BorderLayout(5,5));
                JLabel info = new JLabel(infoString);
                JTextArea t = new JTextArea(itemString);
                JButton close = new JButton("Close Order " + i);
                if(o.closed == false) close.setEnabled(false);
                if(findUserByName(o.user) == -1) close.setEnabled(true);
                t.setEditable(false);
                close.addActionListener(new OrderHandler());
                p.add(info, BorderLayout.NORTH);
                p.add(t, BorderLayout.CENTER);
                p.add(close, BorderLayout.SOUTH);
                orderPanel.add(p);
            }
            
            for( ; end < 8 ; end++) {
                orderPanel.add(new JLabel());
            }
            
        }
        
        
        
        
        
        
        //Process messages
        private void processMessages() {
           
            for(int i = 0 ; i < numUsers ; i++) {  //Loop thru all the users
                LinkedList l = users[i].getMessages();
                
                for(int j = 0 ; j < l.size() ; j++) {  //Loop thru all messages
                    String s = (String)l.removeFirst();
                    StringTokenizer st = new StringTokenizer(s, "~");
                    String opcode = st.nextToken();
                    
                    try {
                        
                        
                        
                        
                        
                        
                    
                    if(opcode.equals("02")) {    ////////////////MSG USER
                        String to = st.nextToken();
                        int u = findUserByName(to);
			if(to.equals(serverName)) {
			    inbox.add(new txtMsg("server" , users[i].userName, st.nextToken()));
                            users[i].sendMessage("02~00");
			}
                        else if(u == -1) {
			    to = to.substring(1,to.length()-1);
			    u = findUserByName(to);
                            if(to.equals(serverName)) {
                                inbox.add(new txtMsg("server" , users[i].userName, st.nextToken()));
                                users[i].sendMessage("02~00");
                            }
			    else if (u != -1) {
				users[u].inbox.add(new txtMsg(to, users[i].userName, st.nextToken()));
                                users[i].sendMessage("02~00");
			    }
                            else users[i].sendMessage("02~07");
                        }
                        else {
                            users[u].inbox.add(new txtMsg(to, users[i].userName, st.nextToken()));
                            users[i].sendMessage("02~00");
                        }
                    }
                    
                    
                    else if(opcode.equals("03")) {  //////////////SEND MOTD
                        String ret = "04~" + motd;
                        if(ret.length() > 1024) {
                            ret = ret.substring(0,1024);
                        }
                        users[i].sendMessage(ret);
                    }
                    
                    
                    else if(opcode.equals("04")) {  ////////////////////RETREIVE INBOX
                        LinkedList msgs = users[i].inbox;
                        for(int k = 0 ; k < msgs.size() ; k++) {
                            txtMsg t = (txtMsg)msgs.get(k);
                            t.unread = false;
                            users[i].sendMessage("03~" + t.from + "~" + t.msg);
                            msgs.set(k,t);
                        }
                        users[i].inbox = msgs;
                        users[i].sendMessage("01");
                    }
                    
                    
                    else if(opcode.equals("05")) {  ////////////////OPEN NEW ORDER
                        String ref = st.nextToken();
                        int r = Integer.valueOf(ref).intValue();
                        int k = findOrderByRef(r);
                        if(k == -1) {  //Open new order with ref
                            orderList.add(new Order(nextOrderNum++,r,users[i].userName));
                            users[i].sendMessage("02~00");
                        }
                        else {  //Order w/ ref is already active, make sure it is not closed
                            Order o = ((Order)orderList.get(k));
                            o.closed = false;
                            orderList.set(k,o);
                            users[i].sendMessage("02~00");
                        }
                    }
                    
                    
                    else if(opcode.equals("06")) {  ///////////////////APPEND ITEM TO ORDER
                        String ref,newItem,newMods;
                        ref = st.nextToken();
                        while(ref.charAt(0) == ' ') {
	                        ref = ref.substring(1);
                        }
                        newItem = st.nextToken();
                        while(newItem.charAt(0) == ' ') {
	                        newItem = newItem.substring(1);
                        }
                        try {newMods = st.nextToken();}
                        catch(Exception e) { newMods = ""; }
                        int r = Integer.valueOf(ref).intValue();
                        int iID = Integer.valueOf(newItem).intValue();
                        int k = findOrderByRef(r);
                        if(k == -1) users[i].sendMessage("02~04");  //Invalid order ref
                        else {
                            int itemNum = findItemByID(iID);
                            if(itemNum == -1) users[i].sendMessage("02~05");  //Bad item id
                            else {
                                Item indexItem = (Item)itemList.get(itemNum);
                                Item nextItem = new Item(indexItem.id,indexItem.name,indexItem.desc,indexItem.price);
                                nextItem.mods = newMods;
                                if(indexItem.countdown == true) {
                                    if(indexItem.counter <= 0)
                                        users[i].sendMessage("02~06");  //Item is 86'd
                                }
                                else {
                                    Order o = (Order)orderList.get(k);
                                    o.add(nextItem);
                                    orderList.set(k,o);
                                    users[i].sendMessage("02~00");
                                    indexItem.counter --;
                                    itemList.set(itemNum,indexItem);
                                }
                            }
                        }
                    }
                    
                    
                    else if(opcode.equals("07")) {  /////////////////////CLOSE ORDER
                        String ref = st.nextToken();
                        int r = Integer.valueOf(ref).intValue();
                        int k = findOrderByRef(r);
                        if(k == -1) {  //Invalid order ref
                            users[i].sendMessage("02~04");
                        }
                        else {
                            Order o = ((Order)orderList.get(k));
                            o.closed = true;
                            orderList.set(k,o);
                            users[i].sendMessage("02~00");
                        }
                        
                    }
                    
                    
                    else if(opcode.equals("08")) {  //////////////GET COMPLETE ITEM LIST
                        for(int k = 0 ; k < itemList.size() ; k++) {
                            Item nextItem = (Item)itemList.get(k);
                            users[i].sendMessage("06~" + nextItem.id + "~" + nextItem.name + "~" + nextItem.desc + "~" + nextItem.mods + "~" + nextItem.price);
                        }
                        users[i].sendMessage("01");
                    }
                    
                    
                    else if(opcode.equals("09")) {  ////////GET ITEM LIST FOR ORDER
                        String ref = st.nextToken();
                        while(ref.charAt(0) == ' ') {
	                        ref = ref.substring(1);
                        }
                        int r = Integer.valueOf(ref).intValue();
                        int m = findOrderByRef(r);
                        if(m >= 0) {
                            Order o = (Order)orderList.get(m);
                            for(int k = 0 ; k < o.size() ; k++) {
                                Item nextItem = (Item)o.get(k);
                                users[i].sendMessage("06~" + nextItem.id + "~" + nextItem.name + "~" + nextItem.desc + "~" + nextItem.mods + "~" + nextItem.price);
                            }
                        }
                        users[i].sendMessage("01");
                    }
                    
                    
                    else if(opcode.equals("10")) {  ////////////GET COMPLETE ORDER LIST
                        for(int k = 0 ; k < orderList.size() ; k ++) {
                            Order o = (Order)orderList.get(k);
                            if((o.user).equals(users[i].userName)) {
				if(!o.closed) users[i].sendMessage("07~" + o.ref);
			    }
                        }
                        users[i].sendMessage("01");
                    }
                    
                    
                    else if(opcode.equals("12")) {  ///////////REMOVE MSG FROM INBOX
                        String msgID = st.nextToken();
                        int m = Integer.valueOf(msgID).intValue();
                        users[i].inbox.remove(m);
                        users[i].sendMessage("02~00");
                    }
                    
                    
                    else if(opcode.equals("13")) {  ///////////////////CHECK FOR NEW MESSAGES
                        int ret = 0;
                        for(int k = 0 ; k < users[i].inbox.size() ; k++) {
                            txtMsg m = (txtMsg)users[i].inbox.get(k);
                            if(m.unread == true) ret++;
                        }
                        users[i].sendMessage("08~" + ret);
                    }
                    
                    else if(opcode.equals("14")) {  /////////////////////GET TOTAL PRICE FOR CHECK
                        String ref = st.nextToken();
                        while(ref.charAt(0) == ' ') {
	                        ref = ref.substring(1);
                        }
                        int r = Integer.valueOf(ref).intValue();
                        int m = findOrderByRef(r);
                        if(m >= 0) {
                            Order o = (Order)orderList.get(m);
                            double total = o.getTotal();
                            users[i].sendMessage("09~" + total);
                        }
                        else users[i].sendMessage("09~000.00");
                    }
                    
                    
                    else {
                        users[i].sendMessage("02~08");
                    }
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    }
                    
                    catch(Exception e) {
                        System.out.println("Error in message from client: " + e.toString());
                        //users[i].close();
                    }
                    
                    
                    
                    
                }  //Loop all messages
                
            }  //Loop all users
            
            
            
        }
        
        
        
        private int findUserByName(String name) {
            for(int i = 0 ; i < numUsers ; i++) {
                if(users[i].userName.equals(name)) return i;
            }
            
            return -1;
        }
        
        
        private int findOrderByRef(int ref) {
            for(int i = 0 ; i < orderList.size() ; i++) {
                if(((Order)orderList.get(i)).ref == ref) return i;
            }
            
            return -1;
        }
        
        
        private int findItemByID(int id) {
            for(int i = 0 ; i < itemList.size() ; i++) {
                if(((Item)itemList.get(i)).id == id) return i;
            }
            
            return -1;
        }
        
        
        
        
        
        
        
        
        ///////////////////////////////
        //Inner dialog windows
        //////////////////////////////
        
        
        //Mgs User dialog
        private class msgDialog extends JFrame {
            
            private final int diaX = 300 , diaY = 300;
            
            JTextField userName;
            JButton ok, cancel;
            JLabel userLabel;
            JPanel userPanel, buttonPanel;
            JTextArea msg;
            JFrame frame = this;
            
            public msgDialog() {
                super("Send a Message");
                Container cont = getContentPane();
                cont.setLayout(new BorderLayout(5,5));
                
                userName = new JTextField("   <enter username>", 15);
                
                userLabel = new JLabel("Username: ");
                
                ok = new JButton("OK");
                cancel = new JButton("Cancel");
                
                msg = new JTextArea("   <enter message>" , 6,60);
                msg.setLineWrap(true);
                
                userPanel = new JPanel(new FlowLayout());
                buttonPanel = new JPanel(new FlowLayout());
                buttonPanel.add(ok);
                buttonPanel.add(cancel);
                ok.addActionListener(new buttonHandler());
                cancel.addActionListener(new buttonHandler());
                cancel.setDefaultCapable(true);
                getRootPane().setDefaultButton(cancel);
                userPanel.add(userLabel);
                userPanel.add(userName);
                
                cont.add(userPanel , BorderLayout.NORTH);
                cont.add(buttonPanel, BorderLayout.SOUTH);
                cont.add(msg, BorderLayout.CENTER);
                
                setSize(diaX,diaY);
                setVisible(true);
            }
            
            private class buttonHandler implements ActionListener {
                
                public void actionPerformed(ActionEvent e) {
                    String whichButton = e.getActionCommand();
                
                    if(whichButton.equals("OK")) {
                        int whichUser = findUserByName(userName.getText());
                        
                        if(whichUser < 0) {  //Could not find user
                            JOptionPane.showMessageDialog(frame,
                                "Error: " + userName.getText() + " is not logged on.",
                                "Could not send message",
                                JOptionPane.ERROR_MESSAGE);
                                closeDialog();
                        }
                        else {
                            users[whichUser].inbox.add(new txtMsg(users[whichUser].userName, serverName, msg.getText()));
                            JOptionPane.showMessageDialog(frame,
                                "Message sucessfully sent to: " + userName.getText(),
                                "Message sent",
                                JOptionPane.INFORMATION_MESSAGE);
                            closeDialog();
                        }
                    }
                   
                    else {
                        closeDialog();
                    }

                }
                
            }
            
            private void closeDialog() {
                setVisible(false);
            }
            
        }
                
        
        
        
        
        
        //MOTD dialog
        private class motdDialog extends JFrame {
            
            private final int diaX = 300 , diaY = 300;
            
            JButton ok, cancel;
            JPanel buttonPanel;
            JTextArea msg;
            JFrame frame = this;
            
            
            public motdDialog() {
                super("Set Message of the Day");
                Container cont = getContentPane();
                cont.setLayout(new BorderLayout(5,5));
                
                ok = new JButton("Set MOTD");
                cancel = new JButton("Cancel");
                
                if(motd == null) msg = new JTextArea("   <enter message>" , 6,60);
                else msg = new JTextArea(motd , 6,60);
                msg.setLineWrap(true);
                
                buttonPanel = new JPanel(new FlowLayout());
                buttonPanel.add(ok);
                buttonPanel.add(cancel);
                ok.addActionListener(new buttonHandler());
                cancel.addActionListener(new buttonHandler());
                cancel.setDefaultCapable(true);
                getRootPane().setDefaultButton(cancel);
                
                cont.add(buttonPanel, BorderLayout.SOUTH);
                cont.add(msg, BorderLayout.CENTER);
                
                setSize(diaX,diaY);
                setVisible(true);
            }
            
            private class buttonHandler implements ActionListener {
                
                public void actionPerformed(ActionEvent e) {
                    String whichButton = e.getActionCommand();
                
                    if(whichButton.equals("Set MOTD")) {
                        motd = msg.getText();
                        JOptionPane.showMessageDialog(frame,
                                "Message of the day sucessfully updated",
                                "MOTD Changed",
                                JOptionPane.INFORMATION_MESSAGE);
                        closeDialog();
                    }
                   
                    else {
                        closeDialog();
                    }

                }
                
            }
            
            private void closeDialog() {
                setVisible(false);
            }
            
        }
        
        
        
        
        
        //Broadcast Message dialog
        private class brdcstDialog extends JFrame {
            
            private final int diaX = 300 , diaY = 300;
            
            JTextField userName;
            JButton ok, cancel;
            JPanel buttonPanel;
            JTextArea msg;
            JFrame frame = this;
            
            
            public brdcstDialog() {
                super("Broadcast Message to all Users");
                Container cont = getContentPane();
                cont.setLayout(new BorderLayout(5,5));
                
                ok = new JButton("Broadcast Message");
                cancel = new JButton("Cancel");
                
                msg = new JTextArea("   <enter message>" , 6,60);
                msg.setLineWrap(true);
                
                buttonPanel = new JPanel(new FlowLayout());
                buttonPanel.add(ok);
                buttonPanel.add(cancel);
                ok.addActionListener(new buttonHandler());
                cancel.addActionListener(new buttonHandler());
                cancel.setDefaultCapable(true);
                getRootPane().setDefaultButton(cancel);
                
                cont.add(buttonPanel, BorderLayout.SOUTH);
                cont.add(msg, BorderLayout.CENTER);
                
                setSize(diaX,diaY);
                setVisible(true);
            }
            
            private class buttonHandler implements ActionListener {
                
                public void actionPerformed(ActionEvent e) {
                    String whichButton = e.getActionCommand();
                
                    if(whichButton.equals("Broadcast Message")) {
                        for(int i = 0 ; i < numUsers ; i++) {
                            users[i].inbox.add(new txtMsg(users[i].userName, serverName, msg.getText()));
                        }
                        JOptionPane.showMessageDialog(frame,
                            "Message has sucessfully been sent to all users.",
                            "Broadcast Complete",
                            JOptionPane.INFORMATION_MESSAGE);
                        closeDialog();
                        
                    }
                   
                    else {
                        closeDialog();
                    }

                }
                
            }
            
            private void closeDialog() {
                setVisible(false);
            }
            
        }
                
        
        
        
        //Inbox window
        
        private class inboxDialog extends JFrame {
            
            private final int diaX = 400 , diaY = 400;
            
            private JButton remove, back;
            private JTextArea msg;
            private JList msgList;
            private JPanel buttonPanel;
            JFrame frame = this;
            
            public inboxDialog() {
                
                super("Server Inbox");
                Container cont = getContentPane();
                cont.setLayout(new BorderLayout(5,5));
                
                remove = new JButton("Remove");
                back = new JButton("Back");
                msg = new JTextArea(6,60);
                msg.setEditable(false);
                msg.setLineWrap(true);
                buttonPanel = new JPanel(new FlowLayout());
                remove.addActionListener(new buttonHandler());
                back.addActionListener(new buttonHandler());
                
                String[] strings = new String[inbox.size()];
                for(int i = 0 ; i < inbox.size() ; i++) {
                    txtMsg t = (txtMsg)inbox.get(i);
                    strings[i] = t.from + " @ " + t.date.toString() + " -- " + t.msg;
                }
                
                if(inbox.size() == 0) msg.setText("No messages to display");
                
                msgList = new JList(strings);
                msgList.setSelectionMode(ListSelectionModel.SINGLE_INTERVAL_SELECTION);
                JScrollPane listScroller = new JScrollPane(msgList);
                listScroller.setPreferredSize(new Dimension(350, 300));
                msgList.setVisibleRowCount(4);
                msgList.setSize(350,150);
                msgList.addListSelectionListener(new listHandler());
                  
                
                buttonPanel.add(remove);
                buttonPanel.add(back);
                cont.add(msgList,BorderLayout.NORTH);
                cont.add(msg,BorderLayout.CENTER);
                cont.add(buttonPanel,BorderLayout.SOUTH);
                
                setSize(diaX,diaY);
                setVisible(true);

                
            }
            
            
            
            private class buttonHandler implements ActionListener {
                
                public void actionPerformed(ActionEvent e) {
                    String whichButton = e.getActionCommand();
                    
                    if(whichButton.equals("Back")) {
                        closeDialog();
                    }
                    else if (whichButton.equals("Remove")) {
                        int which = msgList.getSelectedIndex();
                        if(which >= 0) {
                            inbox.remove(which);
                            JOptionPane.showMessageDialog(frame,
                                "This message has been deleted.",
                                "Removal Complete",
                                JOptionPane.INFORMATION_MESSAGE);
                            closeDialog();
                        }
                        
                    }
                    
                    
                }
                
                
            }
            
            
            
            
            
            private class listHandler implements ListSelectionListener {
                
                public void valueChanged(ListSelectionEvent e) {
                    
                    int which = msgList.getSelectedIndex();
                    if(which >= 0) {
                        txtMsg t = (txtMsg)inbox.get(which);
                        msg.setText(t.msg);
                    }

                    
                }
                
            }
            
            
            
            
            
            
            
            
            private void closeDialog() {
                setVisible(false);
            }
            
            
            
            
            
            
            
            
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    /////////////////////////////////////////////////
    //
    //  PRIVATE CLASS userThread EXTENDS Thread
    //
    //  Responsible for communicating with client and passing messages to the server
    //
    /////////////////////////////////////////////////
    
    private class userThread extends Thread {
        
        private Socket s;
        private BufferedReader in;
        private PrintWriter out;
        private LinkedList incoming = new LinkedList(), 
                           outgoing = new LinkedList();
        private boolean connected;
        private int userNum;
        public String userName;
        public LinkedList inbox = new LinkedList();
        
        
        //  Constructor
        //  Bind socket and setup reader and writer
        private userThread(Socket thisSocket, int id) throws IOException {
            s = thisSocket;
            in = new BufferedReader(new InputStreamReader(s.getInputStream()));
            out = new PrintWriter(s.getOutputStream(), true);  
            connected = true;
            userNum = id;
            try {   //Log the user in
                String login = in.readLine();
                if(login != null) {
                    StringTokenizer t = new StringTokenizer(login , "~");
                    String opcode = t.nextToken();
                    if(opcode.equals("01")) {
                        userName = t.nextToken();
                        String pass = t.nextToken();
                        out.println("02~00");
                    }
                    else {
                        out.println("02~01");  //Login failed
                        close();
                    }
                }
                else {
                    out.println("02~01");  //Login failed
                    close();
                }
            }
            catch(Exception e) {
                    System.out.println("Error communicating with client: " + e.toString());
                    connected = false;
            }    
        }
        
        
        //  Run
        //  Process requests from client
        //  Relay messages from server
        public void run() {
            
            while(connected) {
                try {
                    //Get next message from client
                    String nextMessage = null;
                    if(in.ready() == true) nextMessage = in.readLine();
                    if(nextMessage != null) {
                        incoming.addLast(nextMessage);
                        System.out.println(nextMessage);
                        if(nextMessage.charAt(0) == '1' && nextMessage.charAt(1) == '1') {
                            connected = false;
                            out.println("02~00");  //Disconnect OK
                        }
                    }
                    
                    //Send any pending messages to the client
                    for(int i = 0 ; i < outgoing.size() ; i++) {
                        String t = (String)outgoing.removeFirst();
                        out.println(t);
                        System.out.println("--->" + userName + ":  " + t);
                    }
                }
                
                catch(Exception e) {
                    System.out.println("Error communicating with client: " + e.toString());
                    connected = false;
                }
            }  //while
            
            
            //Close connection if no longer connected
            try { 
                s.close();       
            }                            
            catch (IOException e) {
                System.out.println("Error closing client: " + e.toString());
            }
            removeUser(userNum);
            
        }  //run
        
        
        
        //  getMessages()
        //  Return a LinkedList of new messages from client
        public LinkedList getMessages() {
            LinkedList l = incoming;
            incoming = new LinkedList();
            return l;
        }
        
        
        
        //  sendMessage
        //  Append a message to be sent to the client
        public void sendMessage(String newMessage) {
            outgoing.addLast(newMessage);
            return;
        }
        
        public void sendMessage(String[] newMessages) {
            for(int i = 0 ; i < newMessages.length ; i++)
                outgoing.addLast(newMessages[i]);
            return;
        }
        
        
        
        // close
        // Disconnect user
        public void close() {
            connected = false;
        }
        
        
        
    }  //CLASS userThread
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    //Remove user
    //Synchronized method to update user array when a client closes  
    public synchronized void removeUser(int id) {
        users[id] = null;
        for (int i = id; i < numUsers-1; i++) { 
            users[i] = users[i+1];
        }
        numUsers--;
    }
    
    
    

    
    //  Main
    //  Start the server
    public static void main(String[] args) {
        nodeServer s = new nodeServer();
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    /////////////////////////////////////////////////
    //
    //  private CLASS Item
    //
    /////////////////////////////////////////////////
    
    private class Item implements Cloneable {
        
        public int id;
        public String name, desc, mods;
        public double price;
        public int counter = 0;
        public boolean countdown = false;
        
        public Item(int i, String n, String d, double p) {
            id = i;
            name = n;
            desc = d;
            price = p;
        }
        
    }
    
    
    
    /////////////////////////////////////////////////
    //
    //  private CLASS Order
    //
    /////////////////////////////////////////////////
    
    private class Order extends LinkedList {
        
        public int id, ref;
        public String user;
        public Date date;
        public boolean closed = false;
        
        public Order(int i, int r, String u) {
            super();
            id = i;
            ref = r;
            user = u;
            date = new Date();
        }
        
        public double getTotal() {
            double tot = 0;
            for(int i = 0 ; i < this.size() ; i++) {
                Item thisItem = (Item)this.get(i);
                tot += thisItem.price;
            }
            
            return tot;
        }
        
    }
    
    
    
    
    
    
    /////////////////////////////////////////////////
    //
    //  private CLASS txtMsg
    //
    /////////////////////////////////////////////////
    
    private class txtMsg {
        
        public String to, from , msg;
        public boolean unread = true;
        public Date date;
        
        public txtMsg(String t, String f, String m) {
            to = t;
            from = f;
            msg = m;
            date = new Date();
        }
        
    }
    
    
    
    
    
    
    
    

    
    
}  //class nodeServer



