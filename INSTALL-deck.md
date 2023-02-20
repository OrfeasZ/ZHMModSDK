# Installing on the Steam Deck / Proton

Extract mod contents to the games install directory, the `Retail` directory as instructed:

```
/steamapps/common/HITMAN 3/Retail
```

With use of an application called [Protontricks](https://github.com/Matoking/protontricks) you will be able to modify your wineprefix for the game.

Run Protontricks and select `HITMAN 3: 1659040` from the list
![image](https://user-images.githubusercontent.com/73155407/219853838-52bfc5df-2207-44b3-ac50-6a1bc6c2418c.png)

You may get a few popup warnings by Winetricks about prefix arch type, but you can ignore these warnings. The next window that pops up will ask you to select which prefix, just make sure it's selected on the default option and hit OK.

![image](https://user-images.githubusercontent.com/73155407/219854095-006d1339-a648-4f39-9923-f781ddb7c29c.png)

Next you want to select `Run winecfg` and press OK, to then be greeted with the Wine Configuration
![image](https://user-images.githubusercontent.com/73155407/219854177-5b36e29a-2b44-4305-923a-f5b75656147d.png)

Here you want to go to `Libraries` tab, your DLL overrides list may already be populated, but that's fine, just add these DLL overrides `dinput8` and `zhmmodsdk`, it's lower-case sensitive when you type in the `New override for library` field box, and you don't need to type the `.dll` part when adding, just so long the name matches the DLL files. Make sure they're both set to (Native,Builtin), you can check this by clicking on the DLL name in the list and click Edit.

![image](https://user-images.githubusercontent.com/73155407/219854316-80693e85-042b-42cf-a3ae-2b5d012dc592.png) ![image](https://user-images.githubusercontent.com/73155407/219854375-63debc67-6b19-400c-af21-4c441014ad1a.png)

Once you've done all that hit Apply and close everything and launch the game as normal and it should work.