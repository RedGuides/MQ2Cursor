---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2cursor.114/"
support_link: "https://www.redguides.com/community/threads/mq2cursor.66818/"
repository: "https://github.com/RedGuides/MQ2Cursor"
config: "Server_Character.ini"
authors: "s0rCieR, EqMule, Sym"
tagline: "INI based cursor handling. Auto-keeps specified items, up to a desired quantity. Consumes, Drops, or Destroys items when the desired quantity is reached."
acknowledgements: "BrianMan, A_Druid_00 and Cr4zyb4rd. Maintainers of MQ2AutoDestroy and MQ2Feedme"
---

# MQ2Cursor

<!--desc-start-->
This plugin provides INI based cursor handling. Auto-keeps specified items, up to a desired quantity. Consumes, Drops, or Destroys items when the desired quantity is reached.
<!--desc-end-->
Features:

* Auto Keep item from list up to a certain quantity.
* Consume, Drop, or Destroy when quantity is reached.
* Auto Sleep with certain windows are open (Spellbook, Casting, Bank, Give, Merchant, Trade, GuildTributemaster, TributeMaster, GuildBank, Inventory but not Loot window, Loot window but not Inventory).
* Quiet/Silent Operating Mode and Global On/Off flags.
* Autoloot your corpse when loot Window Up.

## Commands

<a href="cmd-cursor/">
{% 
  include-markdown "projects/mq2cursor/cmd-cursor.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2cursor/cmd-cursor.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2cursor/cmd-cursor.md') }}

<a href="cmd-keep/">
{% 
  include-markdown "projects/mq2cursor/cmd-keep.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2cursor/cmd-keep.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2cursor/cmd-keep.md') }}

## Settings

Example server_character.ini with explanations

```ini
[MQ2Cursor]
Active=1
; Active: 1 is on, 0 is off
Silent=0
; Silent: 1 is silent, 0 is verbose
Random=3
; Random: delay before doing something with item
[MQ2Cursor_ItemList]
9979=Gloomingdeep Lantern|1
; will keep one lantern
21779=Bandages|-1
; will keep all Bandages
58654=Crescent Reach Guild Summons|4
; will keep 4 guild summons
```
