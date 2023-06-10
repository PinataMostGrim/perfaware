ext\ndisasm listings\listing_0037_single_register_mov
ext\ndisasm listings\listing_0038_many_register_mov
ext\ndisasm listings\listing_0039_more_movs
ext\ndisasm listings\listing_0040_challenge_movs
ext\ndisasm listings\listing_0041_add_sub_cmp_jnz

:: F:\Projects\C\perf_aware\part01>ndisasm listing_0037_single_register_mov
:: 00000000  89D9              mov cx,bx

:: F:\Projects\C\perf_aware\part01>ndisasm listing_0038_many_register_mov
:: 00000000  89D9              mov cx,bx
:: 00000002  88E5              mov ch,ah
:: 00000004  89DA              mov dx,bx
:: 00000006  89DE              mov si,bx
:: 00000008  89FB              mov bx,di
:: 0000000A  88C8              mov al,cl
:: 0000000C  88ED              mov ch,ch
:: 0000000E  89C3              mov bx,ax
:: 00000010  89F3              mov bx,si
:: 00000012  89FC              mov sp,di
:: 00000014  89C5              mov bp,ax

:: F:\Projects\C\perf_aware\part01>ndisasm listing_0039_more_movs
:: 00000000  89DE              mov si,bx
:: 00000002  88C6              mov dh,al
:: 00000004  B10C              mov cl,0xc
:: 00000006  B5F4              mov ch,0xf4
:: 00000008  B90C00            mov cx,0xc
:: 0000000B  B9F4FF            mov cx,0xfff4
:: 0000000E  BA6C0F            mov dx,0xf6c
:: 00000011  BA94F0            mov dx,0xf094
:: 00000014  8A00              mov al,[bx+si]
:: 00000016  8B1B              mov bx,[bp+di]
:: 00000018  8B5600            mov dx,[bp+0x0]
:: 0000001B  8A6004            mov ah,[bx+si+0x4]
:: 0000001E  8A808713          mov al,[bx+si+0x1387]
:: 00000022  8909              mov [bx+di],cx
:: 00000024  880A              mov [bp+si],cl
:: 00000026  886E00            mov [bp+0x0],ch

:: F:\Projects\C\perf_aware\part01>ndisasm listing_0040_challenge_movs
:: 00000000  8B41DB            mov ax,[bx+di-0x25]
:: 00000003  898CD4FE          mov [si-0x12c],cx
:: 00000007  8B57E0            mov dx,[bx-0x20]
:: 0000000A  C60307            mov byte [bp+di],0x7
:: 0000000D  C78585035B01      mov word [di+0x385],0x15b
:: 00000013  8B2E0500          mov bp,[0x5]
:: 00000017  8B1E820D          mov bx,[0xd82]
:: 0000001B  A1FB09            mov ax,[0x9fb]
:: 0000001E  A11000            mov ax,[0x10]
:: 00000021  A3FA09            mov [0x9fa],ax
:: 00000024  A30F00            mov [0xf],ax

:: F:\Projects\C\perf_aware\part01>ndisasm listing_0041_add_sub_cmp_jnz
:: 00000000  0318              add bx,[bx+si]
:: 00000002  035E00            add bx,[bp+0x0]
:: 00000005  83C602            add si,byte +0x2
:: 00000008  83C502            add bp,byte +0x2
:: 0000000B  83C108            add cx,byte +0x8
:: 0000000E  035E00            add bx,[bp+0x0]
:: 00000011  034F02            add cx,[bx+0x2]
:: 00000014  027A04            add bh,[bp+si+0x4]
:: 00000017  037B06            add di,[bp+di+0x6]
:: 0000001A  0118              add [bx+si],bx
:: 0000001C  015E00            add [bp+0x0],bx
:: 0000001F  015E00            add [bp+0x0],bx
:: 00000022  014F02            add [bx+0x2],cx
:: 00000025  007A04            add [bp+si+0x4],bh
:: 00000028  017B06            add [bp+di+0x6],di
:: 0000002B  800722            add byte [bx],0x22
:: 0000002E  8382E8031D        add word [bp+si+0x3e8],byte +0x1d
:: 00000033  034600            add ax,[bp+0x0]
:: 00000036  0200              add al,[bx+si]
:: 00000038  01D8              add ax,bx
:: 0000003A  00E0              add al,ah
:: 0000003C  05E803            add ax,0x3e8
:: 0000003F  04E2              add al,0xe2
:: 00000041  0409              add al,0x9
:: 00000043  2B18              sub bx,[bx+si]
:: 00000045  2B5E00            sub bx,[bp+0x0]
:: 00000048  83EE02            sub si,byte +0x2
:: 0000004B  83ED02            sub bp,byte +0x2
:: 0000004E  83E908            sub cx,byte +0x8
:: 00000051  2B5E00            sub bx,[bp+0x0]
:: 00000054  2B4F02            sub cx,[bx+0x2]
:: 00000057  2A7A04            sub bh,[bp+si+0x4]
:: 0000005A  2B7B06            sub di,[bp+di+0x6]
:: 0000005D  2918              sub [bx+si],bx
:: 0000005F  295E00            sub [bp+0x0],bx
:: 00000062  295E00            sub [bp+0x0],bx
:: 00000065  294F02            sub [bx+0x2],cx
:: 00000068  287A04            sub [bp+si+0x4],bh
:: 0000006B  297B06            sub [bp+di+0x6],di
:: 0000006E  802F22            sub byte [bx],0x22
:: 00000071  83291D            sub word [bx+di],byte +0x1d
:: 00000074  2B4600            sub ax,[bp+0x0]
:: 00000077  2A00              sub al,[bx+si]
:: 00000079  29D8              sub ax,bx
:: 0000007B  28E0              sub al,ah
:: 0000007D  2DE803            sub ax,0x3e8
:: 00000080  2CE2              sub al,0xe2
:: 00000082  2C09              sub al,0x9
:: 00000084  3B18              cmp bx,[bx+si]
:: 00000086  3B5E00            cmp bx,[bp+0x0]
:: 00000089  83FE02            cmp si,byte +0x2
:: 0000008C  83FD02            cmp bp,byte +0x2
:: 0000008F  83F908            cmp cx,byte +0x8
:: 00000092  3B5E00            cmp bx,[bp+0x0]
:: 00000095  3B4F02            cmp cx,[bx+0x2]
:: 00000098  3A7A04            cmp bh,[bp+si+0x4]
:: 0000009B  3B7B06            cmp di,[bp+di+0x6]
:: 0000009E  3918              cmp [bx+si],bx
:: 000000A0  395E00            cmp [bp+0x0],bx
:: 000000A3  395E00            cmp [bp+0x0],bx
:: 000000A6  394F02            cmp [bx+0x2],cx
:: 000000A9  387A04            cmp [bp+si+0x4],bh
:: 000000AC  397B06            cmp [bp+di+0x6],di
:: 000000AF  803F22            cmp byte [bx],0x22
:: 000000B2  833EE2121D        cmp word [0x12e2],byte +0x1d
:: 000000B7  3B4600            cmp ax,[bp+0x0]
:: 000000BA  3A00              cmp al,[bx+si]
:: 000000BC  39D8              cmp ax,bx
:: 000000BE  38E0              cmp al,ah
:: 000000C0  3DE803            cmp ax,0x3e8
:: 000000C3  3CE2              cmp al,0xe2
:: 000000C5  3C09              cmp al,0x9
:: 000000C7  7502              jnz 0xcb
:: 000000C9  75FC              jnz 0xc7
:: 000000CB  75FA              jnz 0xc7
:: 000000CD  75FC              jnz 0xcb
:: 000000CF  74FE              jz 0xcf
:: 000000D1  7CFC              jl 0xcf
:: 000000D3  7EFA              jng 0xcf
:: 000000D5  72F8              jc 0xcf
:: 000000D7  76F6              jna 0xcf
:: 000000D9  7AF4              jpe 0xcf
:: 000000DB  70F2              jo 0xcf
:: 000000DD  78F0              js 0xcf
:: 000000DF  75EE              jnz 0xcf
:: 000000E1  7DEC              jnl 0xcf
:: 000000E3  7FEA              jg 0xcf
:: 000000E5  73E8              jnc 0xcf
:: 000000E7  77E6              ja 0xcf
:: 000000E9  7BE4              jpo 0xcf
:: 000000EB  71E2              jno 0xcf
:: 000000ED  79E0              jns 0xcf
:: 000000EF  E2DE              loop 0xcf
:: 000000F1  E1DC              loope 0xcf
:: 000000F3  E0DA              loopne 0xcf
:: 000000F5  E3D8              jcxz 0xcf
